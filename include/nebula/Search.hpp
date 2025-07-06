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
    static constexpr int max_history = 16384;
    
    int max_depth;
    std::vector<std::array<Move, 2>> killers;
    int history[2][64][64];
    int butterfly[2][64][64];
    TranspositionTable tt;

    // mutable Board because of make_move/unmake_move
    int pvs(Board& board, int depth, int alpha, int beta, bool null_move_allowed = true);

    // quiescence search
    int quiesce(Board& board, int depth, int alpha, int beta);

    // move ordering by importance
    void order_moves(std::vector<Move>& moves, Board& board, int depth, const Move* pv_move = nullptr);

    // clear history tables
    void clear_history();

    // scale history to prevent overflow
    void scale_history();

    // update history tables
    void update_history(const Move& move, Color color, int depth, bool cutoff);

    // get history score
    int get_history_score(const Move& move, Color color) const;

    // is move a capture
    inline bool is_capture(const Move& m) const
    {
        return m.flags & static_cast<uint8_t>(MoveFlag::Capture);
    }

    // is move a promotion
    inline bool is_promotion(const Move& m) const
    {
        return m.flags & static_cast<uint8_t>(MoveFlag::Promotion);
    }

    // is move a check
    inline bool gives_check(Board& board, const Move& m) const
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