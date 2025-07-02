#ifndef NEBULA_SEARCH_HPP
#define NEBULA_SEARCH_HPP

#include "nebula/Board.hpp"
#include "nebula/TranspositionTable.hpp"

#include <limits>

namespace nebula
{

class Search
{
public:
    // initialize with maximum depth
    Search(int max);

    // modifies references if there are possible moves, otherwise returns false
    bool best_move(const Board& b, Move& out_best, double& eval);

private:
    static constexpr double infinity = std::numeric_limits<double>::infinity();
    static constexpr double mate_score = 10000.0;

    int max_depth;
    TranspositionTable tt;

    // mutable Board because of make_move/unmake_move
    double pvs(Board& board, int depth, double alpha, double beta, bool null_move_allowed = true);

    // quiescence search
    double quiesce(Board& board, int depth, double alpha, double beta);

    // static evaluation: white +, black -
    double evaluate(const Board& board);

    // move ordering by importance
    void order_moves(std::vector<Move>& moves, Board& board, const Move* pv_move = nullptr);

    // is move a capture
    inline bool is_capture(const Move& m)
    {
        return m.flags & static_cast<uint8_t>(MoveFlag::Capture);
    }

    // is move a promotion
    inline bool is_promotion(const Move& m)
    {
        return m.flags & static_cast<uint8_t>(MoveFlag::Promotion);
    }

    // is move a check
    inline bool gives_check(Board& board, const Move& m)
    {
        board.make_move(m);

        Color foe = board.turn();
        uint64_t king = board.pieces(foe, PieceType::King);

        if(!king)
        {
            board.unmake_move();
            return false;
        }

        int king_sq = __builtin_ctzll(king);
        bool check = board.is_attacked(king_sq, foe == Color::White ? Color::Black : Color::White);

        board.unmake_move();

        return check;
    }
};

}

#endif