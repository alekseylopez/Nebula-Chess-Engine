#include "nebula/Search.hpp"

namespace nebula
{

Search::Search(int max):
    max_depth(max), killers(max_depth + 1, {Move{}, Move{}}) {}

bool Search::best_move(const Board& b, Move& out_best, double& eval)
{
    // lots of operations need a mutable board
    Board board = b;

    std::vector<Move> legal_moves = board.generate_moves();

    if(legal_moves.empty())
        return false;
    
    Move best_move = legal_moves[0];
    int best_eval = -infinity;
    
    // iterative deepening
    for(int depth = 1; depth <= max_depth; ++depth)
    {
        int current_best = -infinity;
        Move current_move = legal_moves[0];
        
        // order moves based on previous iteration results
        order_moves(legal_moves, board, depth);
        
        for(const Move& move : legal_moves)
        {
            board.make_move(move);
            
            int score = -pvs(board, depth - 1, -infinity, infinity);
            
            board.unmake_move();
            
            if(score > current_best)
            {
                current_best = score;
                current_move = move;
            }
        }
        
        // update best move and evaluation
        best_eval = current_best;
        best_move = current_move;
        
        // move the best move to front for next iteration
        auto it = std::find(legal_moves.begin(), legal_moves.end(), best_move);
        if(it != legal_moves.end())
            std::swap(*it, legal_moves[0]);
    }
    
    out_best = best_move;
    eval = static_cast<double>(board.turn() == Color::White ? best_eval : -best_eval) / 100.0;

    return true;
}

int Search::pvs(Board& board, int depth, int alpha, int beta, bool null_move_allowed)
{
    uint64_t key = board.key();

    // draw options
    if(board.is_repetition() || board.is_fifty_move_rule())
        return 0;

    // check transposition table
    const TTEntry* tt_entry = tt.probe(key);
    Move tt_move;
    bool has_tt_move = false;

    if(tt_entry)
    {
        tt_move = tt_entry->move;
        has_tt_move = true;

        if(tt_entry->depth >= depth)
        {
            int tt_score = tt_entry->eval;

            if(tt_entry->flag == TTFlag::Exact)
                return tt_score;
            else if(tt_entry->flag == TTFlag::LowerBound && tt_score >= beta)
                return tt_score;
            else if(tt_entry->flag == TTFlag::UpperBound && tt_score <= alpha)
                return tt_score;
        }
    }

    if(depth == 0)
        return quiesce(board, depth, alpha, beta);

    // null move pruning
    if(null_move_allowed && board.should_try_null_move(depth) && beta < mate_score - 100 && alpha > -mate_score + 100)
    {
        board.make_null_move();

        int score = -pvs(board, depth - 3, -beta, -beta + 1, false);

        board.unmake_null_move();

        // if null move causes beta cutoff, we can prune
        if(score >= beta) // don't return mate scores from null move
            return score >= mate_score - 100 ? beta : score;
    }
    
    std::vector<Move> moves = board.generate_moves();

    // checkmate or stalemate
    if(moves.empty())
        return board.in_check() ? -mate_score + (max_depth - depth) : 0;
    
    order_moves(moves, board, depth, has_tt_move ? &tt_move : nullptr);

    int best_score = -infinity;
    Move best_move = moves[0];
    bool pv_node = true;

    // recursive call for each move
    for(const Move& move : moves)
    {
        board.make_move(move);

        int score;
        
        if(pv_node)
        {
            // search first move with full window
            score = -pvs(board, depth - 1, -beta, -alpha);
        } else
        {
            // null window search first
            score = -pvs(board, depth - 1, -alpha - 1, -alpha);
            
            // if it fails high, re-search with full window
            if(score > alpha && score < beta)
                score = -pvs(board, depth - 1, -beta, -alpha);
        }

        board.unmake_move();
        
        // update score
        if(score > best_score)
        {
            best_score = score;
            best_move = move;
        }

        // beta cutoff (opponent won't allow this)
        if(score >= beta)
        {
            if(!is_capture(move))
            {
                auto& killer = killers[depth];
                
                // shift and store killers
                killer[1] = killer[0];
                killer[0] = move;
            }
            
            tt.store(key, score, depth, TTFlag::LowerBound, move);

            return score;
        }

        // update alpha
        if(score > alpha)
            alpha = score;
        
        pv_node = false;
    }

    tt.store(key, best_score, depth, (best_score <= alpha) ? TTFlag::UpperBound : TTFlag::Exact, best_move);

    return best_score;
}

int Search::quiesce(Board& board, int depth, int alpha, int beta)
{
    int stand_pat = (board.turn() == Color::White) ? evaluate(board) : -evaluate(board);

    // beta cutoff
    if(stand_pat >= beta)
        return stand_pat;
    
    // delta pruning
    if (stand_pat + delta_margin < alpha)
        return stand_pat; 
    
    // update alpha
    if(stand_pat > alpha)
        alpha = stand_pat;
    
    // only keep important moves
    std::vector<Move> important = board.generate_moves();
    important.erase(std::remove_if(important.begin(), important.end(), [&](const Move& m)
    {
        return !is_capture(m) && !is_promotion(m) && !gives_check(board, m);
    }), important.end());

    // no important moves
    if(important.empty())
        return stand_pat;

    order_moves(important, board, depth);

    // recursive call for each imporant move
    for(const Move& move : important)
    {
        // gain approximation
        int gain = 0;
        if(is_capture(move) && move.capture != 0xFF)
            gain = Values::material_value[move.capture & 0b111] - Values::material_value[move.piece & 0b111];

        // delta cutoff
        if(stand_pat + gain + Values::material_value[static_cast<int>(PieceType::Queen)] < alpha)
            continue;
        
        board.make_move(move);

        int score = -quiesce(board, depth + 1, -beta, -alpha);
        
        board.unmake_move();

        // beta cutoff
        if(score >= beta)
            return score;
        
        // update alpha
        if(score > alpha)
            alpha = score;
    }

    return alpha;
}

int Search::evaluate(const Board& board)
{
    int score = 0;
    
    score += material(board);
    
    return score;
}

int Search::material(const Board& board)
{
    int mat = 0;
    int opening_score = 0;
    int endgame_score = 0;

    for(int piece = 0; piece < Board::num_piece_types; ++piece)
    {
        PieceType pt = static_cast<PieceType>(piece);
        
        // white pieces
        uint64_t white_pieces = board.pieces(Color::White, pt);
        while(white_pieces)
        {
            int sq = __builtin_ctzll(white_pieces);
            mat += Values::material_value[piece];
            opening_score += Values::pst[piece][sq];
            endgame_score += Values::pst_endgame[piece][sq];
            white_pieces &= white_pieces - 1;
        }
        
        // black pieces
        uint64_t black_pieces = board.pieces(Color::Black, pt);
        while(black_pieces)
        {
            int sq = __builtin_ctzll(black_pieces);
            mat -= Values::material_value[piece];
            // mirror PST scores vertically
            opening_score -= Values::pst[piece][sq ^ 56];
            endgame_score -= Values::pst[piece][sq ^ 56];
            black_pieces &= black_pieces - 1;
        }
    }

    double phase = phase_of_game(board);
    int blended_pst = static_cast<int>(opening_score * phase + endgame_score * (1.0 - phase));

    return mat + blended_pst;
}

void Search::order_moves(std::vector<Move>& moves, Board& board, int depth, const Move* pv_move)
{
    // assign scores to moves for sorting
    std::vector<std::pair<int, size_t>> move_scores;
    move_scores.reserve(moves.size());

    const std::array<Move,2>* killer = nullptr;
        if (depth >= 0 && depth < (int)killers.size())
            killer = &killers[depth];

    for(size_t i = 0; i < moves.size(); ++i)
    {
        const Move& move = moves[i];
        int score = 0;
        
        // PV move gets highest priority
        if(pv_move && move == *pv_move)
            score += 10000;
        
        if(killer)
        {
            // killer move bonuses
            if(moves[i] == (*killer)[0])
                score += 8000;
            else if(moves[i] == (*killer)[1])
                score += 7000;
        }
        
        // captures get high priority
        if(is_capture(move))
        {
            score += 1000;

            // most valuable victim - least valuable attacker
            if(move.capture != 0xFF)
            {
                int victim_value = Values::material_value[move.capture];
                int attacker_value = Values::material_value[move.piece];
                score += victim_value - attacker_value / 10;
            }
        }
        
        // promotions
        if(is_promotion(move))
        {
            score += 900;
            if(move.promo != 0xFF)
                score += Values::material_value[move.promo] / 10;
        }
        
        // checks
        if(gives_check(board, move))
            score += 50;
        
        // castling moves
        if(move.flags & (static_cast<uint8_t>(MoveFlag::KingCastle) | static_cast<uint8_t>(MoveFlag::QueenCastle)))
            score += 25;
        
        move_scores.emplace_back(score, i);
    }
    
    // sort by score
    std::sort(move_scores.begin(), move_scores.end(), [](const auto& a, const auto& b){ return a.first > b.first; });
    
    // reorder moves based on scores
    std::vector<Move> ordered_moves;
    ordered_moves.reserve(moves.size());
    
    for(const auto& pair : move_scores)
        ordered_moves.push_back(moves[pair.second]);
    
    moves = std::move(ordered_moves);
}

}