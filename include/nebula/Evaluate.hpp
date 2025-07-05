#ifndef EVALUATE_HPP
#define EVALUATE_HPP

#include "Board.hpp"

namespace nebula
{

class Evaluate
{
public:
    // static evaluation
    static int evaluate(const Board& board);

private:
    static constexpr int phase_weight[6] =
    {
        0, // pawn
        1, // knight
        1, // bishop
        2, // rook
        4, // queen
        0  // king
    };
    static constexpr int max_phase = (phase_weight[1] * 2 + phase_weight[2] * 2 + phase_weight[3] * 2 + phase_weight[4] * 1) * 2;
    
    // returns a value in [0, 1]: 1 = full opening, 0 = full endgame
    static inline double phase_of_game(const Board& board)
    {
        int phase = 0;

        // subtract phase-weight for each piece off the board
        for(int pt = 1; pt <= 4; ++pt)
        {
            // count white + black
            int count = __builtin_popcountll(board.pieces(Color::White,  static_cast<PieceType>(pt))) + __builtin_popcountll(board.pieces(Color::Black,  static_cast<PieceType>(pt)));
            phase += (phase_weight[pt] * count);
        }

        // clamp and normalize
        if(phase < 0)
            phase = 0;

        return static_cast<double>(phase) / max_phase;
    }

    // static evaluation helpers
    static int material(const Board& board, double phase);
    static int castling_bonus(const Board& board, double phase);
};

}

#endif