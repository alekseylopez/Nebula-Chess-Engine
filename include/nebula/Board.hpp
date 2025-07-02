#ifndef NEBULA_BOARD_HPP
#define NEBULA_BOARD_HPP

#include <array>
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

namespace nebula
{

enum class MoveFlag : uint8_t
{
    Quiet = 0,
    Capture = 1 << 0,
    DoublePawnPush = 1 << 1,
    EnPassant = 1 << 2,
    KingCastle = 1 << 3,
    QueenCastle = 1 << 4,
    Promotion = 1 << 5,
};

struct Move
{
    static constexpr char pchar[6] = { 'p', 'n', 'b', 'r', 'q', 'k' };

    uint8_t from;
    uint8_t to;
    uint8_t piece; // moving piece code (color << 3 | piece_type)
    uint8_t capture; // 0xFF if none
    uint8_t promo; // 0xFF if none
    uint8_t flags;

    std::string uci() const;

    bool operator==(const Move& other) const;
};

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

    // unicode pieces
    static constexpr const char* piece_unicode[2][6] =
    {
        { u8"♙", u8"♘", u8"♗", u8"♖", u8"♕", u8"♔" },
        { u8"♟", u8"♞", u8"♝", u8"♜", u8"♛", u8"♚" }
    };

    // constructor
    explicit Board(const std::string& fen = std::string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));

    // making moves
    void make_move(const Move& m);
    void unmake_move();
    
    // null moves
    void make_null_move();
    void unmake_null_move();

    // estimate if null move heuristic is a good idea
    bool should_try_null_move(int depth) const;

    // check for repetition
    bool is_repetition() const;

    // check for fifty move rule
    inline bool is_fifty_move_rule() const { return half_moves >= 100; }

    // getting peudo-legal moves
    std::vector<Move> generate_pseudo() const;

    // getting moves
    std::vector<Move> generate_moves();

    // legality check
    bool is_legal(const Move& move);

    // get a bitboard
    inline uint64_t pieces(Color c, PieceType pt) const { return pieces_bb[as_int(c)][as_int(pt)]; }
    inline uint64_t occupancy(Color c) const { return color_bb[as_int(c)]; };
    inline uint64_t occupancy() const { return all_pieces_bb; };

    // returns -1 if empty, otherwise returns (color << 3 | piece_type)
    inline int piece_at(int sq) const { return (unsigned) sq < 64 ? mailbox[sq] : -1; };

    // get king square
    inline int king_sq(Color c) const { return __builtin_ctzll(pieces_bb[as_int(c)][as_int(PieceType::King)]); }

    // returns side to move
    inline Color turn() const { return side_to_move; }

    // get castling rights
    inline int castling() const { return castling_rights; }

    // returns number of full moves
    inline int full() const { return full_move; }

    // helper
    bool is_attacked(int sq, Color by) const;

    // is in check
    bool in_check() const;

    // modifiers
    void set_piece(int sq, Color c, PieceType pt);
    void remove_piece(int sq);

    // for Zobrist hashing
    inline uint64_t key() const { return zobrist_key; }

    // print the board
    void print(std::ostream& os = std::cout) const;

    Move from_uci(const std::string& uci) const;

private:
    // for unmaking moves
    struct Undo
    {
        Move move;
        Color prev_side_to_move;
        int prev_castling_rights;
        int prev_en_passant;
        int prev_half_moves;
        int prev_full_move;
        uint64_t prev_zobrist_key;
    };

    // for unmaking null moves
    struct NullUndo
    {
        Color prev_side_to_move;
        int prev_en_passant;
        uint64_t prev_zobrist_key;
    };

    // history of positions
    std::vector<uint64_t> pos_history;

    // history of moves
    std::vector<Undo> history;

    // history of null moves
    std::vector<NullUndo> null_history;

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

    // all the pieces
    std::array<int, 64> mailbox;
    
    // for Zobrist hashing
    uint64_t zobrist_key;

    static constexpr uint64_t rank_2 = 0xFFULL << 8;
    static constexpr uint64_t rank_7 = 0xFFULL << 48;
    static constexpr uint64_t file_a = 0x0101010101010101ULL;
    static constexpr uint64_t file_h = 0x8080808080808080ULL;

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

    // move generation organization
    void generate_pawn_moves(std::vector<Move>& moves) const;
    void generate_knight_moves(std::vector<Move>& moves) const;
    void generate_king_moves(std::vector<Move>& moves) const;

    // returns -1 if invalid char, otherwise returns 0 and modifies out_c and out_pt
    int piece_char_to_code(char c, Color& out_c, PieceType& out_pt) const;

    // get the integer behind the enumeration
    inline int as_int(MoveFlag mf) const { return static_cast<int>(mf); }
    inline int as_int(Color c) const { return static_cast<int>(c); }
    inline int as_int(PieceType pt) const { return static_cast<int>(pt); }

    // get the enumeration type
    inline Color as_color(int c) const { return static_cast<Color>(c); }
    inline PieceType as_piece_type(int pt) const { return static_cast<PieceType>(pt); }

    // encode piece as an integer
    inline int encode_piece(Color c, PieceType pt) const { return (as_int(c) << 3) | as_int(pt); }

    // get piece info from integer
    inline int decode_color(int piece) const { return piece >> 3; }
    inline int decode_piece(int piece) const { return piece & 0b111; }

    // quiet move helper
    inline Move make_pawn_move(int from, int to, int color, PieceType promo_pt, uint8_t flags) const
    {
        Move m;

        m.from = static_cast<uint8_t>(from);
        m.to = static_cast<uint8_t>(to);
        m.piece = static_cast<uint8_t>((color << 3) | static_cast<uint8_t>(PieceType::Pawn));
        m.flags = flags;
        m.promo = (flags & static_cast<uint8_t>(MoveFlag::Promotion)) ? static_cast<uint8_t>(promo_pt) : static_cast<uint8_t>(0xFF);
        m.capture = static_cast<uint8_t>(0xFF);

        return m;
    }

    // capture helper
    inline Move make_pawn_capture(int from, int to, int color, PieceType pt, uint8_t flags) const
    {
        Move m = make_pawn_move(from, to, color, pt, flags);

        m.capture = static_cast<uint8_t>(mailbox[to]);

        return m;
    }

    // en passant helper
    inline Move make_pawn_ep(int from, int to, int color) const
    {
        Move m;

        m.from = static_cast<uint8_t>(from);
        m.to = static_cast<uint8_t>(to);
        m.piece = static_cast<uint8_t>((color << 3) | int(PieceType::Pawn));
        m.flags = static_cast<uint8_t>(MoveFlag::EnPassant) | static_cast<uint8_t>(MoveFlag::Capture);
        m.promo = static_cast<uint8_t>(0xFF);
        int cap_sq = to + (color == 0 ? -8 : 8);
        m.capture = static_cast<uint8_t>(mailbox[cap_sq]);
        
        return m;
    }

    // normal move helper
    inline Move make_piece_move(int from, int to, int color, PieceType pt, uint8_t flags = static_cast<uint8_t>(MoveFlag::Quiet), uint8_t capture = 0xFF) const
    {
        Move m;

        m.from = static_cast<uint8_t>(from);
        m.to = static_cast<uint8_t>(to);
        m.piece = static_cast<uint8_t>((color << 3) | as_int(pt));
        m.flags = flags;
        m.capture = capture;
        m.promo = 0xFF;

        return m;
    }

    // castling helper
    inline Move make_castle_move(uint8_t flags) const
    {
        Move m;

        m.flags = flags;
        m.capture = 0xFF;
        m.promo = 0xFF;

        if(flags == static_cast<uint8_t>(MoveFlag::KingCastle))
        {
            if(side_to_move == Color::White)
            {
                m.from = 4;
                m.to = 6;
            } else
            {
                m.from = 60;
                m.to = 62;
            }
        } else
        {
            if(side_to_move == Color::White)
            {
                m.from = 4;
                m.to = 2;
            } else
            {
                m.from = 60;
                m.to = 58;
            }
        }
        
        m.piece = static_cast<uint8_t>((as_int(side_to_move) << 3) | as_int(PieceType::King));
        
        return m;
    }

    // allow for initialization of Zobrist hashing arrays
    friend struct ZobristInit;
};

}

#endif