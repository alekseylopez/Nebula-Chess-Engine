#include "nebula/Search.hpp"

namespace nebula
{

TranspositionTable::TranspositionTable(size_t size_mb)
{
    size_t entries = (size_mb * 1024 * 1024) / sizeof(TTEntry);

    size_t table_size = 1;
    
    while(table_size <= entries)
        table_size <<= 1;
    table_size >>= 1;
    
    table.resize(table_size);
    std::fill(table.begin(), table.end(), TTEntry{});

    size_mask = table_size - 1;

    current_age = 0;
}

bool TranspositionTable::probe(uint64_t hash, int depth, double alpha, double beta, double& score, Move& best_move)
{
    TTEntry& entry = table[hash & size_mask];
    
    if(entry.hash != hash)
        return false;
    if(entry.depth < depth)
        return false;

    best_move = entry.best_move;

    if(entry.type == Type::Exact)
    {
        score = entry.score;

        return true;
    } else if(entry.type == Type::LowerBound && entry.score >= beta)
    {
        score = entry.score;

        return true;
    } else if(entry.type == Type::UpperBound && entry.score <= alpha)
    {
        score = entry.score;

        return true;
    }

    return false;
}

void TranspositionTable::store(uint64_t hash, double score, int depth, Type type, Move best_move)
{
    TTEntry& entry = table[hash & size_mask];
    
    if(entry.hash == 0 || entry.hash == hash || depth >= entry.depth || current_age - entry.age > 2)
    {
        entry.hash = hash;
        entry.score = score;
        entry.depth = depth;
        entry.type = type;
        entry.best_move = best_move;
        entry.age = current_age;
    }
}

struct Frame
{
    int depth;
    double alpha, beta;
    double best;
    size_t index;
    std::vector<Move> moves;
    Move tt_move;
    Move best_move;
    uint64_t hash;
    bool tt_probed;
    bool null_move_done;
    bool null_move_allowed;
    bool pv_node;
    bool in_check;
    Phase phase;
    bool needs_research;
    double research_score;
};

struct QFrame
{
    double alpha, beta;
    double best;
    double stand_pat;
    size_t move_index;
    std::vector<Move> moves;
    int depth;
    bool stand_pat_done;
    bool generated_moves;
};

double evaluate(Board& board)
{
    double material = 0;
    
    // material and piece-square tables
    for (int sq = 0; sq < 64; ++sq)
    {
        int code = board.piece_at(sq);

        if(code < 0)
            continue;

        int color = code >> 3;
        int pt = code & 0b111;

        double mat = Values::material_value[pt];
        double pst = Values::pst[pt][sq];
        double val = mat + pst;

        material += (color == 0 ? val : -val);
    }

    // mobility bonus
    auto moves = board.generate_moves();
    int mobility = static_cast<int>(moves.size());

    return material + mobility * 0.1;
}

void order_moves(std::vector<Move>& moves, Board& board)
{
    // score moves
    std::vector<std::pair<int, Move>> scored;
    scored.reserve(moves.size());

    for(auto& m : moves)
    {
        int score = 0;

        if(is_capture(m))
            score += 1000 + (m.capture != 0xFF ? m.capture & 0b111 : 0); // prefer capturing higher value
        if(is_promotion(m))
            score += 100; // promotions are important
        if(gives_check(board, m))
            score += 10; // checks are good

        scored.emplace_back(score, m);
    }

    std::stable_sort(scored.begin(), scored.end(), [](const auto& a, const auto& b)
    {
        // high scores first
        return a.first > b.first;
    });

    // Write back
    for(size_t i = 0; i < moves.size(); ++i)
        moves[i] = scored[i].second;
}

double quiesce(Board& board, double alpha, double beta)
{
    constexpr int max_quiesce_depth = 6; // Reduced from 8
    constexpr double delta_margin = 9.0;

    std::vector<QFrame> stack;
    stack.reserve(max_quiesce_depth + 2);
    
    stack.push_back(
    {
        alpha, beta,
        -1e9,
        -1e9,
        0,
        {},
        0,
        false,
        false
    });

    double ret = 0.0;

    while(!stack.empty())
    {
        QFrame& frame = stack.back();

        // safety check
        if(frame.depth >= max_quiesce_depth)
        {
            ret = evaluate(board);

            stack.pop_back();
            
            if(!stack.empty())
            {
                QFrame& prev = stack.back();

                double value = -ret;
                if(value > prev.best)
                    prev.best = value;
                if(prev.best > prev.alpha)
                    prev.alpha = prev.best;
                
                board.unmake_move();

                ++prev.move_index;
            }

            continue;
        }

        // Initialize frame on first entry
        if(!frame.stand_pat_done)
        {
            frame.stand_pat_done = true;
            frame.stand_pat = evaluate(board);

            // Beta cutoff
            if(frame.stand_pat >= frame.beta)
            {
                ret = frame.beta;
                stack.pop_back();

                if(!stack.empty())
                {
                    QFrame& prev = stack.back();

                    double value = -ret;
                    if(value > prev.best)
                        prev.best = value;
                    if(prev.best > prev.alpha)
                        prev.alpha = prev.best;

                    board.unmake_move();

                    ++prev.move_index;
                }

                continue;
            }

            // delta pruning
            if(frame.stand_pat + delta_margin < frame.alpha)
            {
                ret = frame.alpha;

                stack.pop_back();
                
                if(!stack.empty())
                {
                    QFrame& prev = stack.back();

                    double value = -ret;
                    if(value > prev.best)
                        prev.best = value;
                    if(prev.best > prev.alpha)
                        prev.alpha = prev.best;

                    board.unmake_move();

                    ++prev.move_index;
                }

                continue;
            }

            if(frame.stand_pat > frame.alpha)
                frame.alpha = frame.stand_pat;
            
            frame.best = frame.alpha;
        }

        // generate moves once
        if(!frame.generated_moves)
        {
            frame.generated_moves = true;

            auto all_moves = board.generate_moves();
            
            // only include captures and promotions
            for(const auto& m : all_moves)
                if(is_capture(m) || is_promotion(m))
                    frame.moves.push_back(m);

            order_moves(frame.moves, board);
        }

        // check if we're done with moves
        if(frame.move_index >= frame.moves.size())
        {
            ret = frame.best;
            stack.pop_back();

            if(!stack.empty())
            {
                QFrame& prev = stack.back();

                double value = -ret;
                if(value > prev.best)
                    prev.best = value;
                if(prev.best > prev.alpha)
                    prev.alpha = prev.best;

                board.unmake_move();

                ++prev.move_index;
            }

            continue;
        }

        // beta pruning
        if(frame.best >= frame.beta)
        {
            ret = frame.best;
            stack.pop_back();

            if(!stack.empty())
            {
                QFrame& prev = stack.back();

                double value = -ret;
                if(value > prev.best)
                    prev.best = value;
                if(prev.best > prev.alpha)
                    prev.alpha = prev.best;

                board.unmake_move();

                ++prev.move_index;
            }

            continue;
        }

        const Move& current_move = frame.moves[frame.move_index];
        board.make_move(current_move);

        stack.push_back(
        {
            -frame.beta, 
            -std::max(frame.alpha, frame.best),
            -1e9,
            -1e9,
            0,
            {},
            frame.depth + 1,
            false,
            false
        });
    }

    return ret;
}

double negamax(Board& board, int max_depth)
{
    constexpr double inf = 1e9;
    constexpr double mate_score = 1000000.0;

    std::vector<Frame> stack;
    stack.push_back(
    {
        max_depth,
        -inf, inf,
        -inf,
        0,
        {},
        Move{},
        Move{},
        0,
        false,
        false,
        true,
        true,
        false,
        Phase::Init,
        false,
        0.0
    });

    double ret = 0.0;

    while(!stack.empty())
    {
        Frame& frame = stack.back();

        switch(frame.phase)
        {
            case Phase::Init:
            {
                frame.hash = board.key();
                frame.in_check = board.in_check();
                frame.pv_node = (frame.beta - frame.alpha > 1);
                
                // quiescence search
                if(frame.depth == 0)
                {
                    ret = quiesce(board, frame.alpha, frame.beta);
                    stack.pop_back();
                    
                    if(!stack.empty())
                    {
                        Frame& prev = stack.back();

                        double value = -ret;
                        if(value > prev.best)
                        {
                            prev.best = value;
                            prev.best_move = prev.moves.empty() ? Move{} : prev.moves[prev.index];
                        }
                        if(prev.best > prev.alpha)
                            prev.alpha = prev.best;
                        
                        board.unmake_move();

                        ++prev.index;
                        prev.phase = Phase::SearchMoves;
                    }

                    continue;
                }
                
                frame.phase = Phase::TTProbe;

                break;
            }

            case Phase::TTProbe:
            {
                double tt_score;

                if(tt.probe(frame.hash, frame.depth, frame.alpha, frame.beta, tt_score, frame.tt_move))
                {
                    ret = tt_score;

                    stack.pop_back();
                    
                    if(!stack.empty())
                    {
                        Frame& prev = stack.back();

                        double value = -ret;
                        if (value > prev.best)
                        {
                            prev.best = value;
                            prev.best_move = prev.moves.empty() ? Move{} : prev.moves[prev.index];
                        }
                        if(prev.best > prev.alpha)
                            prev.alpha = prev.best;
                        board.unmake_move();

                        ++prev.index;
                        prev.phase = Phase::SearchMoves;
                    }
                    continue;
                }
                
                frame.tt_probed = true;
                frame.phase = Phase::NullMove;

                break;
            }

            case Phase::NullMove:
            {
                if(frame.null_move_allowed && frame.depth >= 3 && !frame.in_check)
                {
                    const int null_move_reduction = 2;

                    frame.null_move_done = true;
                    
                    board.make_null_move();
                    
                    stack.push_back(
                    {
                        frame.depth - null_move_reduction - 1,
                        -frame.beta, -frame.beta + 1,
                        -inf,
                        0,
                        {},
                        Move{},
                        Move{},
                        0,
                        false,
                        false,
                        false,
                        false,
                        false,
                        Phase::Init,
                        false,
                        0.0
                    });

                    continue;
                }
                
                frame.phase = Phase::MoveGen;

                break;
            }

            case Phase::MoveGen:
            {
                if (frame.null_move_done)
                {
                    board.unmake_null_move();
                    
                    if(-ret >= frame.beta)
                    {
                        ret = frame.beta;

                        stack.pop_back();
                        
                        if(!stack.empty())
                        {
                            Frame& prev = stack.back();

                            double value = -ret;
                            if (value > prev.best)
                            {
                                prev.best = value;
                                prev.best_move = prev.moves.empty() ? Move{} : prev.moves[prev.index];
                            }
                            if (prev.best > prev.alpha)
                                prev.alpha = prev.best;

                            board.unmake_move();

                            ++prev.index;
                            prev.phase = Phase::SearchMoves;
                        }

                        continue;
                    }

                    frame.null_move_done = false;
                }

                // generate and order moves
                frame.moves = board.generate_moves();
                
                // terminal position check
                if(frame.moves.empty())
                {
                    Color us = board.turn();
                    int king_sq = __builtin_ctzll(board.pieces(us, PieceType::King));
                    
                    if(board.is_attacked(king_sq, us == Color::White ? Color::Black : Color::White))
                        ret = -mate_score + (max_depth - frame.depth); // checkmate
                    else
                        ret = 0.0; // stalemate

                    stack.pop_back();
                    
                    if (!stack.empty())
                    {
                        Frame& prev = stack.back();

                        double value = -ret;
                        if (value > prev.best)
                        {
                            prev.best = value;
                            prev.best_move = prev.moves.empty() ? Move{} : prev.moves[prev.index];
                        }
                        if (prev.best > prev.alpha)
                            prev.alpha = prev.best;

                        board.unmake_move();

                        ++prev.index;
                        prev.phase = Phase::SearchMoves;
                    }
                    continue;
                }

                order_moves(frame.moves, board);

                frame.best_move = frame.moves[0];
                frame.phase = Phase::SearchMoves;

                break;
            }

            case Phase::SearchMoves:
            {
                if(frame.index >= frame.moves.size())
                {
                    // store in transposition table
                    Type tt_type = (frame.best <= frame.alpha) ? Type::UpperBound : Type::Exact;
                    tt.store(frame.hash, frame.best, frame.depth, tt_type, frame.best_move);
                    
                    ret = frame.best;

                    stack.pop_back();
                    
                    if(!stack.empty())
                    {
                        Frame& prev = stack.back();

                        double value = -ret;
                        if (value > prev.best)
                        {
                            prev.best = value;
                            prev.best_move = prev.moves.empty() ? Move{} : prev.moves[prev.index];
                        }
                        if(prev.best > prev.alpha)
                            prev.alpha = prev.best;

                        board.unmake_move();

                        ++prev.index;
                    }

                    continue;
                }

                // beta cutoff
                if(frame.best >= frame.beta)
                {
                    // store in transposition table
                    tt.store(frame.hash, frame.best, frame.depth, Type::LowerBound, frame.best_move);
                    
                    ret = frame.beta;

                    stack.pop_back();
                    
                    if(!stack.empty())
                    {
                        Frame& prev = stack.back();

                        double value = -ret;
                        if(value > prev.best)
                        {
                            prev.best = value;
                            prev.best_move = prev.moves.empty() ? Move{} : prev.moves[prev.index];
                        }
                        if(prev.best > prev.alpha)
                            prev.alpha = prev.best;

                        board.unmake_move();

                        ++prev.index;
                    }

                    continue;
                }

                // make move and search
                const Move& current_move = frame.moves[frame.index];
                board.make_move(current_move);
                
                // principal variation search
                if(frame.index == 0 || !frame.pv_node)
                {
                    // First move or non-PV node: full window search
                    stack.push_back(
                    {
                        frame.depth - 1,
                        -frame.beta, -std::max(frame.alpha, frame.best),
                        -inf,
                        0,
                        {},
                        Move{},
                        Move{},
                        0,
                        false,
                        false,
                        true,
                        false,
                        false,
                        Phase::Init,
                        false,
                        0.0
                    });
                } else
                {
                    // principal variation node
                    double search_alpha = std::max(frame.alpha, frame.best);
                    stack.push_back(
                    {
                        frame.depth - 1,
                        -search_alpha - 1, -search_alpha,
                        -inf,
                        0,
                        {},
                        Move{},
                        Move{},
                        0,
                        false,
                        false,
                        true,
                        false,
                        false,
                        Phase::Init,
                        false,
                        0.0
                    });

                    frame.needs_research = true;
                    frame.research_score = 0.0;
                }

                continue;
            }

            default:
            {
                double score = -ret;
    
                if(frame.needs_research)
                {
                    frame.research_score = score;
                    
                    // check if we need full window re-search
                    if(score > frame.alpha && score < frame.beta)
                    {
                        // keep move on board, do full window search
                        stack.push_back(
                        {
                            frame.depth - 1,
                            -frame.beta, -frame.alpha,
                            -inf,
                            0,
                            {},
                            Move{},
                            Move{},
                            0,
                            false,
                            false,
                            true,
                            false,
                            false,
                            Phase::Init,
                            false,
                            0.0
                        });

                        frame.needs_research = false;

                        continue;
                    } else
                    {
                        // null window result is sufficient
                        frame.needs_research = false;
                    }
                }
                
                // Now we can safely unmake the move
                board.unmake_move();
                
                // Update best score and move
                if(score > frame.best)
                {
                    frame.best = score;
                    frame.best_move = frame.moves[frame.index];
                }
                if(frame.best > frame.alpha)
                    frame.alpha = frame.best;
                
                ++frame.index;
                frame.phase = Phase::SearchMoves;

                break;
            }
        }
    }

    return ret;
}

double search_root(Board& board, int max_depth, Move& out_best, SearchResult& result_flag)
{
    constexpr double inf = 1e9;

    auto moves = board.generate_moves();
    order_moves(moves, board);

    if(moves.empty())
    {
        // terminal position
        Color us = board.turn();
        int king_sq = -1;
        for(int sq = 0; sq < 64; ++sq)
            if(board.piece_at(sq) == ((static_cast<int>(us) << 3) | static_cast<int>(PieceType::King)))
                king_sq = sq;

        if(king_sq != -1 && board.is_attacked(king_sq, us == Color::White ? Color::Black : Color::White))
            result_flag = SearchResult::Checkmate;
        else
            result_flag = SearchResult::Stalemate;

        return evaluate(board);
    }
    
    result_flag = SearchResult::KeepGoing;

    double best = -inf;
    Move best_move = moves[0];

    // different root nodes
    for(const Move& m : moves)
    {
        board.make_move(m);
        double score = -negamax(board, max_depth - 1);
        board.unmake_move();

        if(score > best)
        {
            best = score;
            best_move = m;
        }
    }

    out_best = best_move;

    return best;
}

}