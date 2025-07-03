#ifndef NEBULA_SEARCH_HPP
#define NEBULA_SEARCH_HPP

#include "nebula/Board.hpp"
#include "nebula/TranspositionTable.hpp"
#include "nebula/Values.hpp"

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
    static constexpr int infinity = 1000000;
    static constexpr int mate_score = 100000;
    static constexpr int delta_margin = Values::material_value[static_cast<int>(PieceType::Queen)];
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

    int max_depth;
    std::vector<std::array<Move, 2>> killers;
    TranspositionTable tt;

    // mutable Board because of make_move/unmake_move
    int pvs(Board& board, int depth, int alpha, int beta, bool null_move_allowed = true);

    // quiescence search
    int quiesce(Board& board, int depth, int alpha, int beta);

    // static evaluation
    int evaluate(const Board& board);

    // static evaluation helpers
    int material(const Board& board, double phase);
    int castling_bonus(const Board& board, double phase);

    // returns a value in [0, 1]: 1 = full opening, 0 = full endgame
    inline double phase_of_game(const Board& board)
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

    // move ordering by importance
    void order_moves(std::vector<Move>& moves, Board& board, int depth, const Move* pv_move = nullptr);

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