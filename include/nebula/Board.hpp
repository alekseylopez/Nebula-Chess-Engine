#ifndef NEBULA_BOARD_HPP
#define NEBULA_BOARD_HPP

#include <array>
#include <cstdint>
#include <string>

namespace nebula
{

enum class Color : int { White = 0, Black };
enum class PieceType : int { Pawn = 0, Knight, Bishop, Rook, Queen, King };

class Board
{
public:
    // constants for enumerations
    static constexpr int num_colors = 2;
    static constexpr int num_piece_types = 6;

    // constants for castling bit manipulation
    static constexpr int castle_K = 1 << 0;
    static constexpr int castle_Q = 1 << 1;
    static constexpr int castle_k = 1 << 2;
    static constexpr int castle_q = 1 << 3;

    // constructors
    Board();
    explicit Board(const std::string& fen);

    // get a bitboard
    uint64_t pieces(Color c, PieceType pt) const;
    uint64_t occupancy(Color c) const;
    uint64_t occupancy() const;

    // returns -1 if empty, otherwise returns (color * num_piece_types + pieceType)
    int piece_at(int sq) const;

    // modifiers
    void set_piece(int sq, Color c, PieceType pt);
    void remove_piece(int sq);

    // for Zobrist hashing
    inline uint64_t key() const { return zobrist_key; }

private:
    // bitboards
    std::array<std::array<uint64_t, num_piece_types>, num_colors> pieces_bb;
    std::array<uint64_t, num_colors> color_bb;
    uint64_t all_pieces_bb;

    // other data
    Color side_to_move;
    int castling_rights;
    int en_passant_square;
    int half_moves;
    int full_move;
    
    // for Zobrist hashing
    uint64_t zobrist_key;

    // seed for Zobrist hashing initialization
    static constexpr uint64_t seed = 0x9E3779B97F4A7C15ULL;

    // Zobrist hashing arrays
    static uint64_t zobrist_piece[num_colors][num_piece_types][64];
    static uint64_t zobrist_castling[16];
    static uint64_t zobrist_en_passant_file[8];
    static uint64_t zobrist_black_to_move;

    // Zobrist updates
    inline void update_zobrist_piece(int sq, Color c, PieceType pt) { zobrist_key ^= zobrist_piece[as_int(c)][as_int(pt)][sq]; }
    inline void update_zobrist_side() { zobrist_key ^= zobrist_black_to_move; }
    inline void update_zobrist_castling(int oldR, int newR) { zobrist_key ^= zobrist_castling[oldR]; zobrist_key ^= zobrist_castling[newR]; }
    inline void update_zobrist_enpassant(int oldSq, int newSq) { if(oldSq >= 0) zobrist_key ^= zobrist_en_passant_file[oldSq & 7]; if(newSq >= 0) zobrist_key ^= zobrist_en_passant_file[newSq & 7]; }

    // returns -1 if invalid char, otherwise returns 0 and modifies out_c and out_pt
    int piece_char_to_code(char c, Color& out_c, PieceType& out_pt) const;

    // get the integer behind the enumeration
    inline int as_int(Color c) const { return static_cast<int>(c); }
    inline int as_int(PieceType pt) const { return static_cast<int>(pt); }

    // get the enumeration type
    inline Color as_color(int c) const { return static_cast<Color>(c); }
    inline PieceType as_piece_type(int pt) const { return static_cast<PieceType>(pt); }

    // allow for initialization of Zobrist hashing arrays
    friend struct ZobristInit;
};

}

#endif