#ifndef SEARCH_HPP
#define SEARCH_HPP

#include "nebula/Board.hpp"

#include <cstdint>

namespace nebula
{

struct Values
{
    static constexpr double material_value[6] =
    {
        1.0, // Pawn
        3.2, // Knight
        3.3, // Bishop
        5.0, // Rook
        9.0, // Queen
        0.0  // King
    };

    static constexpr double pst[6][64] =
    {
        {
            0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,
            5.0,  5.0,  5.0, -5.0, -5.0,  5.0,  5.0,  5.0,
            1.0,  1.0,  2.0,  3.0,  3.0,  2.0,  1.0,  1.0,
            0.5,  0.5,  1.0,  2.5,  2.5,  1.0,  0.5,  0.5,
            0.0,  0.0,  0.0,  2.0,  2.0,  0.0,  0.0,  0.0,
            0.5, -0.5, -1.0,  0.0,  0.0, -1.0, -0.5,  0.5,
            0.5,  1.0,  1.0, -2.0, -2.0,  1.0,  1.0,  0.5,
            0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0
        },
        {
            -5.0, -4.0, -3.0, -3.0, -3.0, -3.0, -4.0, -5.0,
            -4.0, -2.0,  0.0,  0.5,  0.5,  0.0, -2.0, -4.0,
            -3.0,  0.5,  1.0,  1.5,  1.5,  1.0,  0.5, -3.0,
            -3.0,  0.0,  1.5,  2.0,  2.0,  1.5,  0.0, -3.0,
            -3.0,  0.5,  1.5,  2.0,  2.0,  1.5,  0.5, -3.0,
            -3.0,  0.0,  1.0,  1.5,  1.5,  1.0,  0.0, -3.0,
            -4.0, -2.0,  0.0,  0.0,  0.0,  0.0, -2.0, -4.0,
            -5.0, -4.0, -3.0, -3.0, -3.0, -3.0, -4.0, -5.0
        },
        {
            -2.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -2.0,
            -1.0,  0.5,  0.0,  0.0,  0.0,  0.0,  0.5, -1.0,
            -1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0, -1.0,
            -1.0,  0.0,  1.0,  1.5,  1.5,  1.0,  0.0, -1.0,
            -1.0,  0.5,  0.5,  1.5,  1.5,  0.5,  0.5, -1.0,
            -1.0,  0.0,  0.5,  1.0,  1.0,  0.5,  0.0, -1.0,
            -1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -1.0,
            -2.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -2.0
        },
        {
             0.0,  0.0,  0.0,  0.5,  0.5,  0.0,  0.0,  0.0,
            -0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5,
            -0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5,
            -0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5,
            -0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5,
            -0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, -0.5,
             0.5,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  0.5,
             0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0
        },
        {
            -2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0,
            -1.0,  0.0,  0.5,  0.0,  0.0,  0.5,  0.0, -1.0,
            -1.0,  0.5,  0.5,  0.5,  0.5,  0.5,  0.5, -1.0,
            -0.5,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -0.5,
             0.0,  0.0,  0.5,  0.5,  0.5,  0.5,  0.0, -0.5,
            -1.0,  0.5,  0.5,  0.5,  0.5,  0.5,  0.5, -1.0,
            -1.0,  0.0,  0.5,  0.0,  0.0,  0.5,  0.0, -1.0,
            -2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0
        },
        {
            -3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0,
            -3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0,
            -3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0,
            -3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0,
            -2.0, -3.0, -3.0, -4.0, -4.0, -3.0, -3.0, -2.0,
            -1.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -1.0,
             2.0,  2.0,  0.0,  0.0,  0.0,  0.0,  2.0,  2.0,
             2.0,  3.0,  1.0,  0.0,  0.0,  1.0,  3.0,  2.0
        }
    };
};

enum class SearchResult { KeepGoing, Stalemate, Checkmate };

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
    int king_sq = __builtin_ctzll(board.pieces(foe, PieceType::King));
    bool check = board.is_attacked(king_sq, foe == Color::White ? Color::Black : Color::White);
    board.unmake_move();
    return check;
}

// static evaluation: white +, black -
double evaluate(const Board& board);

// move ordering by importance
void order_moves(std::vector<Move>& moves, Board& board);

// quiescence search
double quiesce(Board& board, double alpha, double beta);

// mutable Board because of make_move/unmake_move
// returns score from white's perspective
double negamax(Board &board, int max_depth);

// returns score of best line
double search_root(Board& board, int max_depth, Move& out_best, SearchResult& result_flag);

}

#endif