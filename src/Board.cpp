#include "nebula/Board.hpp"
#include "nebula/AttackTables.hpp"

#include <random>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <ostream>

namespace nebula
{

uint64_t Board::zobrist_piece[Board::num_colors][Board::num_piece_types][64];
uint64_t Board::zobrist_castling[16];
uint64_t Board::zobrist_en_passant_file[8];
uint64_t Board::zobrist_black_to_move;

struct ZobristInit
{
    ZobristInit()
    {
        std::mt19937_64 rng(Board::seed);

        // fill piece
        for(auto& c : Board::zobrist_piece)
            for(auto& pt : c)
                for(auto& sq : pt)
                    sq = rng();

        // fill castling
        for(auto& x : Board::zobrist_castling)
            x = rng();

        // fill en passant
        for(auto& x : Board::zobrist_en_passant_file)
            x = rng();

        // black to move
        Board::zobrist_black_to_move = rng();
    }
} _zInit;

std::string Move::uci() const
{
    auto sq_to_str = [&](int sq)
    {
        char file = char('a' + (sq & 0b111));
        char rank = char('1' + (sq >> 3));

        return std::string{file, rank};
    };

    std::string s = sq_to_str(from) + sq_to_str(to);

    if(flags & static_cast<uint8_t>(MoveFlag::Promotion))
        s += pchar[promo];
    
    return s;
}

bool Move::operator==(const Move& other) const
{
    return from == other.from && to == other.to && piece == other.piece && capture == other.capture && promo == other.promo && flags == other.flags;
}

Board::Board(const std::string& fen):
    history{},
    pieces_bb{{{0ULL}}},
    color_bb{{0ULL}},
    all_pieces_bb{0ULL},
    side_to_move{Color::White},
    castling_rights{0},
    en_passant_square{-1},
    half_moves{0},
    full_move{1},
    mailbox{},
    zobrist_key{0ULL}
{
    // reset mailbox
    mailbox.fill(-1);

    // split up the string
    std::istringstream iss(fen);
    std::string board, active, castle, ep, halfm, fullm;
    if(!(iss >> board >> active >> castle >> ep >> halfm >> fullm))
        throw std::invalid_argument("Invalid FEN: not enough fields");
    
    // read the pieces
    int rank = 7, file = 0;
    for(char ch : board)
    {
        if(ch == '/') // new rank
        {
            if(file != 8)
                throw std::invalid_argument("Invalid FEN: invalid rank length");
            
            --rank;
            file = 0;
        } else if(isdigit(ch)) // empty space
        {
            file += ch - '0';
        } else // piece
        {
            if(file >= 8 || rank < 0)
                throw std::invalid_argument("Invalid FEN: piece out of bounds");
            
            Color c;
            PieceType pt;

            if(piece_char_to_code(ch, c, pt) < 0)
                throw std::invalid_argument(std::string("Invalid FEN: unrecognized piece: ") + ch);
            
            int sq = rank * 8 + file;

            set_piece(sq, c, pt);

            ++file;
        }
    }
    if(rank != 0 || file != 8)
        throw std::invalid_argument("Invalid FEN: invalid dimensions");
    
    // white or black to move
    side_to_move = (active == "w" ? Color::White : Color::Black);

    // castling rights
    if(castle.find('K') != std::string::npos)
        castling_rights |= castle_K;
    if(castle.find('Q') != std::string::npos)
        castling_rights |= castle_Q;
    if(castle.find('k') != std::string::npos)
        castling_rights |= castle_k;
    if(castle.find('q') != std::string::npos)
        castling_rights |= castle_q;
    
    // en passant
    if(ep != "-")
    {
        int ep_file = ep[0] - 'a';
        int ep_rank = ep[1] - '1';

        en_passant_square = ep_rank * 8 + ep_file;
    }

    // counters
    half_moves = std::stoi(halfm);
    full_move = std::stoi(fullm);
}

void Board::make_move(const Move& move)
{
    // push to history
    history.push_back(
    {
        move,
        side_to_move,
        castling_rights,
        en_passant_square,
        half_moves,
        full_move,
        zobrist_key
    });

    // update half move counter
    if(decode_piece(move.piece) == as_int(PieceType::Pawn) || (move.flags & as_int(MoveFlag::Capture)))
        half_moves = 0;
    else
        ++half_moves;
    
    // update full move counter
    if(side_to_move == Color::Black)
        ++full_move;
    
    // clear en passant
    update_zobrist_enpassant(en_passant_square, -1);
    en_passant_square = -1;

    // make new castling rights
    int new_castling = castling_rights;

    // lose castling rights if kings moves
    if(decode_piece(move.piece) == as_int(PieceType::King))
    {
        if(decode_color(move.piece) == 0)
            new_castling &= ~(castle_K | castle_Q);
        else
            new_castling &= ~(castle_k | castle_q);
    }

    // lose castling rights if rook moves or is captured
    if((move.from == 0) || (move.to == 0))
        new_castling &= ~castle_Q;
    if((move.from == 7) || (move.to == 7))
        new_castling &= ~castle_K;
    if((move.from == 56)|| (move.to == 56))
        new_castling &= ~castle_q;
    if((move.from == 63)|| (move.to == 63))
        new_castling &= ~castle_k;

    // update castling rights
    update_zobrist_castling(castling_rights, new_castling);
    castling_rights = new_castling;

    // handle capture
    if(move.flags & as_int(MoveFlag::EnPassant))
    {
        int cap_sq = move.to + (side_to_move == Color::White ? -8 : 8);
        remove_piece(cap_sq);
    } else if(move.flags & as_int(MoveFlag::Capture))
    {
        remove_piece(move.to);
    }

    // remove piece
    remove_piece(move.from);

    // set the right piece type (handling promotion)
    PieceType drop_pt = as_piece_type(decode_piece(move.piece));
    if(move.flags & as_int(MoveFlag::Promotion))
        drop_pt = as_piece_type(move.promo);
    set_piece(move.to, side_to_move, drop_pt);

    // handle castling rook movement
    if(move.flags & as_int(MoveFlag::KingCastle))
    {
      int rfrom = (side_to_move==Color::White ? 7 : 63);
      int rto = (side_to_move==Color::White ? 5 : 61);

      remove_piece(rfrom);
      set_piece(rto, side_to_move, PieceType::Rook);
    } else if(move.flags & as_int(MoveFlag::QueenCastle))
    {
      int rfrom = (side_to_move==Color::White ? 0 : 56);
      int rto = (side_to_move==Color::White ? 3 : 59);
      
      remove_piece(rfrom);
      set_piece(rto, side_to_move, PieceType::Rook);
    }

    if(move.flags & as_int(MoveFlag::DoublePawnPush))
    {
        int ep = (move.from + move.to) >> 1;
        update_zobrist_enpassant(-1, ep);
        en_passant_square = ep;
    }

    // update side
    update_zobrist_side();
    side_to_move = (side_to_move == Color::White ? Color::Black : Color::White);
}

void Board::unmake_move()
{
    Undo u = history.back();
    history.pop_back();
    const Move& move = u.move;

    side_to_move = u.prev_side_to_move;
    castling_rights = u.prev_castling_rights;
    en_passant_square = u.prev_en_passant;
    half_moves = u.prev_half_moves;
    full_move = u.prev_full_move;
    zobrist_key = u.prev_zobrist_key;

    // clear without modifying Zobrist key
    auto clear_sq = [&](int sq)
    {
        // check that spot
        int code = mailbox[sq];

        // return if empty
        if(code < 0)
            return;

        // get color and piece type
        int c  = decode_color(code);
        int pt = decode_piece(code);

        // translate square number to mask
        const uint64_t mask = 1ULL << sq;

        // modify bitmasks
        pieces_bb[c][pt] &= ~mask;
        color_bb[c] &= ~mask;
        all_pieces_bb &= ~mask;

        // modify mailbox
        mailbox[sq] = -1;
    };

    // add without modifying Zobrist key
    auto add_sq = [&](int sq, int code)
    {
        // get color and piece type
        int c = decode_color(code);
        int pt = decode_piece(code);

        // translate square number to mask
        const uint64_t mask = 1ULL << sq;

        // modify bitmasks
        pieces_bb[c][pt] |= mask;
        color_bb[c] |= mask;
        all_pieces_bb |= mask;

        // modify mailbox
        mailbox[sq] = code;
    };

    clear_sq(move.to);
    if(move.flags & as_int(MoveFlag::Promotion))
    {
        // handle promotions
        add_sq(move.from, move.piece); // move.piece is always the pawn

        if(move.flags & as_int(MoveFlag::Capture))
            add_sq(move.to, move.capture);
    } else if(move.flags & as_int(MoveFlag::EnPassant))
    {
        // handle en passant
        add_sq(move.from, move.piece);
        int capSq = move.to + ((decode_color(move.piece) == 0) ? -8 : 8);
        add_sq(capSq, move.capture);
    } else if(move.flags & as_int(MoveFlag::Capture))
    {
        // handle captures
        add_sq(move.from, move.piece);
        add_sq(move.to, move.capture);
    } else if(move.flags & as_int(MoveFlag::KingCastle))
    {
        // handle castling
        int rfrom = (as_int(side_to_move) == 0 ? 7 : 63);
        int rto = (as_int(side_to_move) == 0 ? 5 : 61);
        clear_sq(rto);
        add_sq(rfrom, encode_piece(side_to_move, PieceType::Rook));
        add_sq(move.from, move.piece);
    } else if(move.flags & as_int(MoveFlag::QueenCastle))
    {
        // more castling
        int rfrom = (as_int(side_to_move) == 0 ? 0 : 56);
        int rto = (as_int(side_to_move) == 0 ? 3 : 59);
        clear_sq(rto);
        add_sq(rfrom, encode_piece(side_to_move, PieceType::Rook));
        add_sq(move.from, move.piece);
    } else
    {
        // regular move
        add_sq(move.from, move.piece);
    }
}

void Board::make_null_move()
{
    // save current state for unmake
    null_history.push_back(
    {
        side_to_move,
        en_passant_square,
        zobrist_key
    });

    // clear en passant square
    update_zobrist_enpassant(en_passant_square, -1);
    en_passant_square = -1;

    // switch sides
    update_zobrist_side();
    side_to_move = (side_to_move == Color::White ? Color::Black : Color::White);
}

void Board::unmake_null_move()
{
    if(null_history.empty())
        return;

    NullUndo u = null_history.back();
    null_history.pop_back();

    // restore previous state
    side_to_move = u.prev_side_to_move;
    en_passant_square = u.prev_en_passant;
    zobrist_key = u.prev_zobrist_key;
}

std::vector<Move> Board::generate_pseudo() const
{
    std::vector<Move> out;

    int color = as_int(side_to_move);
    uint64_t pawns = pieces_bb[color][as_int(PieceType::Pawn)];
    uint64_t empty = ~all_pieces_bb;
    uint64_t enemy_occ = color_bb[color ^ 1];

    constexpr uint64_t rank_2 = 0xFFULL << 8;
    constexpr uint64_t rank_7 = 0xFFULL << 48;
    constexpr uint64_t file_a = 0x0101010101010101ULL;
    constexpr uint64_t file_h = 0x8080808080808080ULL;

    // single pushes
    uint64_t single = color == 0 ? (pawns << 8) : (pawns >> 8);
    single &= empty;
    while(single)
    {
        int to = __builtin_ctzll(single);
        single &= single - 1;

        int from = to + (color == 0 ? -8 : 8);

        int rank = to >> 3;
        if(rank == 7 || rank == 0)
        {
            for(auto pt : { PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight})
                out.push_back(make_pawn_move(from, to, color, pt, static_cast<uint8_t>(MoveFlag::Promotion)));
        } else
        {
            out.push_back(make_pawn_move(from, to, color, PieceType::Pawn, static_cast<uint8_t>(MoveFlag::Quiet)));
        }
    }

    // double pushes
    uint64_t start = pawns & (color == 0 ? rank_2 : rank_7);
    uint64_t one_step = color == 0 ? ((start << 8) & empty) : ((start >> 8) & empty);
    uint64_t dbl = color == 0 ? ((one_step << 8) & empty) : ((one_step >> 8) & empty);
    while(dbl)
    {
        int to = __builtin_ctzll(dbl);
        dbl &= dbl - 1;

        int from = to + (color == 0 ? -16 : 16);
        out.push_back(make_pawn_move(from, to, color, PieceType::Pawn, static_cast<uint8_t>(MoveFlag::DoublePawnPush)));
    }

    // captures
    uint64_t pawns_cp = pawns;
    while(pawns_cp)
    {
        int from = __builtin_ctzll(pawns_cp);
        pawns_cp &= pawns_cp - 1;

        uint64_t attacks = AttackTables::pawn[color][from] & enemy_occ;
        while(attacks)
        {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;

            int rank = to >> 3;
            if(rank == 7 || rank == 0)
            {
                for(auto promo : { PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight })
                    out.push_back(make_pawn_capture(from, to, color, promo, static_cast<uint8_t>(MoveFlag::Capture) | static_cast<uint8_t>(MoveFlag::Promotion)));
            } else
            {
                out.push_back(make_pawn_capture(from, to, color, PieceType::Pawn, static_cast<uint8_t>(MoveFlag::Capture)));
            }
        }
    }

    // en passant
    if(en_passant_square >= 0)
    {
        uint64_t ep_mask = 1ULL << en_passant_square;
        uint64_t from_squares;
        if(color == 0)
            from_squares = ((ep_mask >> 7) & ~file_a) | ((ep_mask >> 9) & ~file_h);
        else
            from_squares = ((ep_mask << 7) & ~file_h) | ((ep_mask << 9) & ~file_a);

        from_squares &= pawns;

        while(from_squares)
        {
            int from = __builtin_ctzll(from_squares);
            from_squares &= from_squares - 1;
            out.push_back(make_pawn_ep(from, en_passant_square, color));
        }
    }

    // knights
    uint64_t knights = pieces_bb[color][as_int(PieceType::Knight)];
    uint64_t me = color_bb[color];
    while(knights)
    {
        int from = __builtin_ctzll(knights);
        knights &= knights - 1;

        uint64_t attacks = AttackTables::knight[from] & ~me;
        while(attacks)
        {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;

            bool is_cap = (all_pieces_bb & (1ULL << to)) && ((color_bb[color ^ 1] >> to) & 1);
            out.push_back(make_piece_move(from, to, color, PieceType::Knight, static_cast<uint8_t>(is_cap ? MoveFlag::Capture : MoveFlag::Quiet), static_cast<uint8_t>(is_cap ? mailbox[to] : 0xFF)));
        }
    }

    // lambda for sliding pieces
    auto slide = [&](PieceType pt, const auto& dirs)
    {
        uint64_t bb = pieces_bb[color][as_int(pt)];
        uint64_t me = color_bb[color];
        uint64_t foe = color_bb[color^1];

        while(bb)
        {
            int from = __builtin_ctzll(bb);
            bb &= bb - 1;

            int f0 = from & 0b111;
            int r0 = from >> 3;

            for(auto [df, dr] : dirs)
            {
                int f = f0 + df, r = r0 + dr;
                while(f >= 0 && f < 8 && r >= 0 && r < 8)
                {
                    int to = r * 8 + f;
                    uint64_t mask = 1ULL << to;

                    // stop
                    if(me & mask)
                        break;

                    // capture and stop
                    if(foe & mask)
                    {
                        out.push_back(make_piece_move(from, to, color, pt, static_cast<uint8_t>(MoveFlag::Capture), static_cast<uint8_t>(mailbox[to])));
                        break;
                    }
                    
                    // keep sliding
                    out.push_back(make_piece_move(from, to, color, pt, static_cast<uint8_t>(MoveFlag::Quiet)));
                    
                    f += df;
                    r += dr;
                }
            }
        }
    };

    slide(PieceType::Rook, AttackTables::rook_dirs);
    slide(PieceType::Bishop, AttackTables::bishop_dirs);
    slide(PieceType::Queen, AttackTables::queen_dirs);

    // kings
    uint64_t kings = pieces_bb[color][as_int(PieceType::King)];
    if(kings)
    {
        int from = __builtin_ctzll(kings);
        uint64_t attacks = AttackTables::king[from] & ~color_bb[color];
        while(attacks)
        {
            int to = __builtin_ctzll(attacks);
            attacks &= attacks - 1;

            bool isCap = ((color_bb[color ^ 1] >> to) & 1);
            out.push_back(make_piece_move(from, to, color, PieceType::King, static_cast<uint8_t>(isCap ? MoveFlag::Capture : MoveFlag::Quiet), static_cast<uint8_t>(isCap ? mailbox[to] : 0xFF)));
        }
    }

    // castling
    if(color == as_int(Color::White))
    {
        if((castling_rights & castle_K) && !(all_pieces_bb & ((1ULL << 5) | (1ULL << 6))) && !is_attacked(4, Color::Black) && !is_attacked(5, Color::Black) && !is_attacked(6, Color::Black))
            out.push_back(make_castle_move(static_cast<uint8_t>(MoveFlag::KingCastle)));

        if((castling_rights & castle_Q) && !(all_pieces_bb & ((1ULL << 1) | (1ULL << 2) | (1ULL << 3))) && !is_attacked(4, Color::Black) && !is_attacked(3, Color::Black) && !is_attacked(2, Color::Black))
            out.push_back(make_castle_move(static_cast<uint8_t>(MoveFlag::QueenCastle)));
    } else
    {
        if((castling_rights & castle_k) && !(all_pieces_bb & ((1ULL << 61) | (1ULL << 62))) && !is_attacked(60, Color::White) && !is_attacked(61, Color::White) && !is_attacked(62, Color::White))
            out.push_back(make_castle_move(static_cast<uint8_t>(MoveFlag::KingCastle)));
        
        if((castling_rights & castle_q) && !(all_pieces_bb & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59))) && !is_attacked(60, Color::White) && !is_attacked(59, Color::White) && !is_attacked(58, Color::White))
            out.push_back(make_castle_move(static_cast<uint8_t>(MoveFlag::QueenCastle)));
    }

    return out;
}

std::vector<Move> Board::generate_moves()
{
    std::vector<Move> pseudo = generate_pseudo();

    std::vector<Move> legal;
    legal.reserve(pseudo.size());

    Color us = side_to_move;
    Color foe = (us == Color::White ? Color::Black : Color::White);

    for(const auto& move : pseudo)
    {
        make_move(move);

        if(!is_attacked(__builtin_ctzll(pieces_bb[as_int(us)][as_int(PieceType::King)]), foe))
            legal.push_back(move);
        
        unmake_move();
    }

    return legal;
}

bool Board::is_attacked(int sq, Color by) const
{
    if(sq < 0 || sq >= 64)
        return false;

    const uint64_t target = 1ULL << sq;
    const uint64_t occ = all_pieces_bb;
    const int c = as_int(by);

    // preventing wrap-around
    static constexpr uint64_t not_file_a = ~0x0101010101010101ULL;
    static constexpr uint64_t not_file_h = ~0x8080808080808080ULL;

    // pawn attacks
    uint64_t pawns = pieces_bb[c][as_int(PieceType::Pawn)];
    uint64_t attacks = 0ULL;

    if (by == Color::White)
        attacks = ((pawns << 7) & not_file_h) | ((pawns << 9) & not_file_a);
    else
        attacks = ((pawns >> 9) & not_file_h) | ((pawns >> 7) & not_file_a);

    if(attacks & target)
        return true;

    // knight attacks
    if (AttackTables::knight[sq] & pieces_bb[c][as_int(PieceType::Knight)])
        return true;

    // king attacks
    if (AttackTables::king[sq] & pieces_bb[c][as_int(PieceType::King)])
        return true;
    
    int f0 = sq & 7, r0 = sq >> 3;

    // rook-like sliding attacks
    for(auto [df, dr] : AttackTables::rook_dirs)
    {
        int f = f0 + df, r = r0 + dr;
        while(f >= 0 && f < 8 && r >= 0 && r < 8)
        {
            int t = (r << 3) | f;
            uint64_t m = 1ULL << t;

            // hit a piece
            if(occ & m)
            {
                int code = mailbox[t];

                int col = decode_color(code), pc = decode_piece(code);

                if(col == c && (pc == as_int(PieceType::Rook) || pc == as_int(PieceType::Queen)))
                    return true;

                break;
            }

            f += df; r += dr;
        }
    }

    // bishop-like sliding attacks
    for(auto [df, dr] : AttackTables::bishop_dirs)
    {
        int f = f0 + df, r = r0 + dr;
        while(f >= 0 && f < 8 && r >= 0 && r < 8)
        {
            int t = (r << 3) | f;
            uint64_t m = 1ULL << t;

            // hit a piece
            if(occ & m)
            {
                int code = mailbox[t];

                int col = decode_color(code), pc = decode_piece(code);

                if(col == c && (pc == as_int(PieceType::Bishop) || pc == as_int(PieceType::Queen)))
                    return true;

                break;
            }

            f += df; r += dr;
        }
    }

    return false;
}

bool Board::in_check() const
{
    Color us = side_to_move;
    Color foe = (us == Color::White ? Color::Black : Color::White);
    
    uint64_t king_bb = pieces_bb[as_int(us)][as_int(PieceType::King)];

    if(!king_bb)
        return false;

    int king_sq = __builtin_ctzll(king_bb);

    return is_attacked(king_sq, foe);
}

void Board::set_piece(int sq, Color c, PieceType pt)
{
    // if invalid return
    if(sq < 0 || sq >= 64)
        return;
    
    // remove anything that was already there
    remove_piece(sq);

    // translate square number to mask
    const uint64_t mask = 1ULL << sq;

    // place the piece
    pieces_bb[as_int(c)][as_int(pt)] |= mask;
    color_bb[as_int(c)] |= mask;
    all_pieces_bb |= mask;

    // place in mailbox
    mailbox[sq] = encode_piece(c, pt);

    // update Zobrist key
    update_zobrist_piece(sq, c, pt);
}

void Board::remove_piece(int sq)
{
    // if invalid return
    if(sq < 0 || sq >= 64)
        return;
    
    // get the type of piece
    int piece = piece_at(sq);

    // if already empty return
    if(piece < 0)
        return;

    // translate square number to mask
    const uint64_t mask = 1ULL << sq;

    // get type and color of piece
    int c = decode_color(piece);
    int pt = decode_piece(piece);

    // remove that piece
    pieces_bb[c][pt] &= ~mask;
    color_bb[c] &= ~mask;
    all_pieces_bb &= ~mask;

    // place in mailbox
    mailbox[sq] = -1;

    // update Zobrist key
    update_zobrist_piece(sq, as_color(c), as_piece_type(pt));
}

void Board::print(std::ostream& os) const
{
    // print the board
    os << "\n  +---+---+---+---+---+---+---+---+\n";

    for(int rank = 7; rank >= 0; --rank)
    {
        os << char('1' + rank) << ' ';

        for(int file = 0; file < 8; ++file)
        {
            int sq = rank * 8 + file;
            int code = mailbox[sq];

            if(code < 0)
            {
                os << "|   ";
            } else
            {
                int c = decode_color(code);
                int pt = decode_piece(code);
                os <<  "| " << piece_unicode[c][pt] << ' ';
            }
        }

        os << "|\n  +---+---+---+---+---+---+---+---+\n";
    }

    os << "    a   b   c   d   e   f   g   h\n\n";

    // side to move
    os << "Side to move: " << (side_to_move == Color::White ? 'w' : 'b') << '\n';

    // castling rights
    os << "Castling: ";
    bool any = false;
    if(castling_rights & castle_K)
    {
        os << 'K';
        any = true;
    }
    if(castling_rights & castle_Q)
    {
        os << 'Q';
        any = true;
    }
    if(castling_rights & castle_k)
    {
        os << 'k';
        any = true;
    }
    if(castling_rights & castle_q)
    {
        os << 'q';
        any = true;
    }
    if(!any)
        os << '-';
    os << '\n';

    // en passant
    os << "En-passant: ";
    if(en_passant_square >= 0)
    {
        char file_c = char('a' + (en_passant_square & 0b111));
        char rank_c = char('1' + (en_passant_square >> 3));
        os << file_c << rank_c;
    } else
    {
        os << '-';
    }
    os << '\n';

    // halfmove and fullmove clocks
    os << "Halfmoves: " + std::to_string(half_moves) + '\n';
    os << "Fullmoves: " + std::to_string(full_move) + '\n';

    os << '\n';
}

Move Board::from_uci(const std::string& uci) const
{
    if(uci.size() < 4)
        throw std::invalid_argument("bad UCI");

    auto str_to_sq = [&](int i)
    {
        char f = uci[i], r = uci[i + 1];

        if(f < 'a' || f > 'h' || r < '1' || r > '8')
            throw std::invalid_argument("bad square");

        return (r - '1') * 8 + (f - 'a');
    };

    int from = str_to_sq(0), to = str_to_sq(2);

    // figure out which piece is moving
    int pc = piece_at(from);
    if(pc < 0)
        throw std::invalid_argument("no piece on from-square");

    Move m;
    m.from = from;
    m.to = to;
    m.piece = static_cast<uint8_t>(pc);
    m.capture = static_cast<uint8_t>((piece_at(to) >= 0)? piece_at(to): 0xFF);
    m.flags = 0;
    m.promo = 0xFF;

    // promotion
    if(uci.size() == 5)
    {
        char q = std::tolower(uci[4]);
        PieceType pt;
        switch (q)
        {
            case 'q':
                pt = PieceType::Queen;
                break;

            case 'r':
                pt = PieceType::Rook;
                break;

            case 'b':
                pt = PieceType::Bishop;
                break;

            case 'n':
                pt = PieceType::Knight;
                break;

            default:
                throw std::invalid_argument("bad promo");
        }
        m.flags |= static_cast<uint8_t>(MoveFlag::Promotion);
        m.promo  = static_cast<uint8_t>(pt);
    }

    // capture flag
    if(m.capture != 0xFF)
        m.flags |= static_cast<uint8_t>(MoveFlag::Capture);

    // double-pawn push
    int dist = std::abs(to - from);
    if((decode_piece(pc) == as_int(PieceType::Pawn)) && dist == 16)
        m.flags |= as_int(MoveFlag::DoublePawnPush);

    // king castling
    if(decode_piece(pc) == as_int(PieceType::King))
    {
        if(from == 4 && to == 6)
            m.flags |= static_cast<uint8_t>(MoveFlag::KingCastle);
        if(from == 4 && to == 2)
            m.flags |= static_cast<uint8_t>(MoveFlag::QueenCastle);
        if(from == 60 && to == 62)
            m.flags |= static_cast<uint8_t>(MoveFlag::KingCastle);
        if(from == 60 && to == 58)
            m.flags |= static_cast<uint8_t>(MoveFlag::QueenCastle);
    }

    return m;
}

int Board::piece_char_to_code(char c, Color& out_c, PieceType& out_pt) const
{
    // determine piece color
    if(std::isupper(static_cast<unsigned char>(c)))
        out_c = Color::White;
    else if(std::islower(static_cast<unsigned char>(c)))
        out_c = Color::Black;
    else
        return -1;

    // standardize
    char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    // determine type
    switch(lower)
    {
        case 'p':
            out_pt = PieceType::Pawn;
            break;

        case 'n':
            out_pt = PieceType::Knight;
            break;

        case 'b':
            out_pt = PieceType::Bishop;
            break;

        case 'r':
            out_pt = PieceType::Rook;
            break;

        case 'q':
            out_pt = PieceType::Queen;
            break;

        case 'k':
            out_pt = PieceType::King;
            break;

        default:
            return -1;
    }

    // successful
    return 0;
}

}