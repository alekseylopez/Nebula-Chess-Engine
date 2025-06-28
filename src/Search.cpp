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
            ret = evaluate(board);

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
            frame.moves = board.generate_moves();

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