#include "nebula/Search.hpp"

namespace nebula
{

struct Frame
{
    int depth;
    double alpha, beta;
    double best;
    size_t index;
    std::vector<Move> moves;
};

struct QFrame
{
    double alpha, beta;
    double best;
    size_t index;
    std::vector<Move> moves;
    bool stand_pat_done;
};

double evaluate(const Board& board)
{
    double material = 0;

    for(int sq = 0; sq < 64; ++sq)
    {
        int code = board.piece_at(sq);

        if(code < 0)
            continue;

        int color = code >> 3;          
        int pt = code & 0b111;     

        double mat = Values::material_value[pt];
        double pst = Values::pst[static_cast<int>(pt)][sq];

        double val = mat + pst;

        material += (color == 0 ? val : -val);
    }

    return material;
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
    std::vector<QFrame> stack;
    stack.push_back(
    {
        alpha, beta,
        -1e9,
        0,
        {},
        false
    });

    double ret = 0.0;

    while(!stack.empty())
    {
        QFrame& frame = stack.back();

        // start of each frame
        if(!frame.stand_pat_done)
        {
            frame.stand_pat_done = true;

            double stand_pat = evaluate(board);

            if(stand_pat >= frame.beta)
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
                    
                    ++prev.index;
                }

                continue;
            }

            if(stand_pat > frame.alpha)
                frame.alpha = stand_pat;
            
            frame.best = frame.alpha;
        }

        // first entry
        if(frame.moves.empty())
        {
            auto moves = board.generate_moves();
            for(const auto& m : moves)
                if(is_capture(m) || is_promotion(m))
                    frame.moves.push_back(m);

            order_moves(frame.moves, board);
        }

        // return best if nothing left
        if(frame.index >= frame.moves.size())
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

                ++prev.index;
            }

            continue;
        }

        // alpha-beta pruning
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

                ++prev.index;
            }

            continue;
        }

        board.make_move(frame.moves[frame.index]);

        stack.push_back(
        {
            -frame.beta, -std::max(frame.alpha, frame.best),
            -1e9,
            0,
            {},
            false
        });

        continue;
    }

    return ret;
}

double negamax(Board& board, int max_depth)
{
    constexpr double inf = 1e9;
    constexpr double mate = 1000000.0;

    std::vector<Frame> stack;
    stack.push_back(
    {
        max_depth,
        -inf, inf,
        -inf,
        0,
        {}
    });

    double ret = 0.0;

    while(!stack.empty())
    {
        Frame& frame = stack.back();

        // leaf node
        if(frame.depth == 0)
        {
            ret = quiesce(board, frame.alpha, frame.beta);

            stack.pop_back();

            if(!stack.empty())
            {
                Frame& prev = stack.back();
                
                double value = -ret;
                if(value > prev.best)
                    prev.best = value;
                if(prev.best > prev.alpha)
                    prev.alpha = prev.best;

                board.unmake_move();

                ++prev.index;
            }

            continue;
        }

        // generate moves
        if(frame.moves.empty())
        {
            frame.moves = board.generate_moves();
            order_moves(frame.moves, board);
        }

        // terminal node
        if(frame.moves.empty())
        {
            Color us = board.turn();
            int king_sq = -1;

            for(int sq = 0; sq < 64; ++sq)
                if (board.piece_at(sq) == ((static_cast<int>(us) << 3) | static_cast<int>(PieceType::King)))
                    king_sq = sq;

            if(king_sq != -1 && board.is_attacked(king_sq, us == Color::White ? Color::Black : Color::White))
                ret = -mate + (max_depth - frame.depth); // checkmate
            else
                ret = 0.0; // stalemate

            stack.pop_back();

            if(!stack.empty())
            {
                Frame& prev = stack.back();

                double value = -ret;
                if (value > prev.best)
                    prev.best = value;
                if (prev.best > prev.alpha)
                    prev.alpha = prev.best;

                board.unmake_move();

                ++prev.index;
            }

            continue;
        }

        // next move
        if(frame.index < frame.moves.size())
        {
            // alpha-beta cutoff before search
            if(frame.best >= frame.beta)
            {
                // beta cutoff
                ret = frame.best;

                stack.pop_back();

                if(!stack.empty())
                {
                    Frame& prev = stack.back();

                    double value = -ret;
                    if (value > prev.best)
                        prev.best = value;
                    if (prev.best > prev.alpha)
                        prev.alpha = prev.best;

                    board.unmake_move();

                    ++prev.index;
                }

                continue;
            }

            board.make_move(frame.moves[frame.index]);

            stack.push_back(
            {
                frame.depth - 1,
                -frame.beta, -std::max(frame.alpha, frame.best),
                -inf,
                0,
                {}
            });

            continue;
        } else
        {
            // searched everything already
            ret = frame.best;

            stack.pop_back();

            if(!stack.empty())
            {
                Frame& prev = stack.back();

                double value = -ret;
                if (value > prev.best)
                    prev.best = value;
                if (prev.best > prev.alpha)
                    prev.alpha = prev.best;

                board.unmake_move();

                ++prev.index;
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