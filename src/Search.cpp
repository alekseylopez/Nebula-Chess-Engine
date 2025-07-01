#include "nebula/Search.hpp"
#include "nebula/Values.hpp"

namespace nebula
{

Search::Search(int max):
    max_depth(max) {}

bool Search::best_move(const Board& b, Move& out_best, double& eval)
{
    // lots of operations need a mutable board
    Board board = b;

    std::vector<Move> legal_moves = board.generate_moves();

    if(legal_moves.empty())
        return false;
    
    Move best_move = legal_moves[0];
    double best_eval = -infinity;
    
    // iterative deepening
    for(int depth = 1; depth <= max_depth; ++depth)
    {
        double current_best = -infinity;
        Move current_move = legal_moves[0];
        
        // order moves based on previous iteration results
        order_moves(legal_moves, board);
        
        for(const Move& move : legal_moves)
        {
            board.make_move(move);
            
            double score = -negamax(board, depth - 1, -infinity, infinity);
            
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
    eval = board.turn() == Color::White ? best_eval : -best_eval;

    return true;
}

double Search::negamax(Board& board, int depth, double alpha, double beta)
{
    uint64_t key = board.key();

    // repeating moves is a draw
    if(board.is_repetition())
    {
        std::cout << "DRAW HERE\n";
        return 0.0;
    }

    // check transposition table
    const TTEntry* tt_entry = tt.probe(key);

    if(tt_entry && tt_entry->depth >= depth)
    {
        double tt_score = tt_entry->eval;

        if(tt_entry->flag == TTFlag::Exact)
            return tt_score;
        else if(tt_entry->flag == TTFlag::LowerBound && tt_score >= beta)
            return tt_score;
        else if(tt_entry->flag == TTFlag::UpperBound && tt_score <= alpha)
            return tt_score;
    }

    if(depth == 0)
        return quiesce(board, depth, alpha, beta);
    
    std::vector<Move> moves = board.generate_moves();

    // checkmate or stalemate
    if(moves.empty())
        return board.in_check() ? -mate_score + (max_depth - depth) : 0.0;
    
    order_moves(moves, board);

    double best_score = -infinity;
    Move best_move = moves[0];

    // recursive call for each move
    for(const Move& move : moves)
    {
        board.make_move(move);

        position_history.push_back(board.key());

        double score = -negamax(board, depth - 1, -beta, -alpha);

        position_history.pop_back();

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
            tt.store(key, score, depth, TTFlag::LowerBound, move);

            return score;
        }

        // update alpha
        if(score > alpha)
            alpha = score;
    }

    tt.store(key, best_score, depth, (best_score <= alpha) ? TTFlag::UpperBound : TTFlag::Exact, best_move);

    return best_score;
}

double Search::quiesce(Board& board, int depth, double alpha, double beta)
{
    double stand_pat = (board.turn() == Color::White) ? evaluate(board) : -evaluate(board);

    // reasonable
    if(depth > 10)
        return stand_pat;

    // beta cutoff
    if(stand_pat >= beta)
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

    order_moves(important, board);

    // recursive call for each imporant move
    for(const Move& move : important)
    {
        board.make_move(move);

        double score = -quiesce(board, depth + 1, -beta, -alpha);
        
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

double Search::evaluate(const Board& board)
{
    double score = 0.0;
    
    for(int piece = 0; piece < Board::num_piece_types; ++piece)
    {
        PieceType pt = static_cast<PieceType>(piece);
        
        // white pieces
        uint64_t white_pieces = board.pieces(Color::White, pt);
        while(white_pieces)
        {
            int sq = __builtin_ctzll(white_pieces);
            score += Values::material_value[piece];
            score += Values::pst[piece][sq] * 0.1;
            white_pieces &= white_pieces - 1;
        }
        
        // black pieces
        uint64_t black_pieces = board.pieces(Color::Black, pt);
        while(black_pieces)
        {
            int sq = __builtin_ctzll(black_pieces);
            score -= Values::material_value[piece];
            // mirror PST scores vertically
            score -= Values::pst[piece][sq ^ 56] * 0.1;
            black_pieces &= black_pieces - 1;
        }
    }
    
    return score;
}

void Search::order_moves(std::vector<Move>& moves, Board& board)
{
    // captures first, then promotions, then others
    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b)
    {
        int score_a = 0, score_b = 0;
        
        // prioritize captures
        if(is_capture(a))
            score_a += 1000;
        if(is_capture(b))
            score_b += 1000;
        
        // then promotions
        if(is_promotion(a))
            score_a += 100;
        if(is_promotion(b))
            score_b += 100;
        
        // then checks
        if(gives_check(board, a))
            score_a += 10;
        if(gives_check(board, b))
            score_b += 10;
        
        return score_a > score_b;
    });
}

}