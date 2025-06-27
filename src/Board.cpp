#include "nebula/Board.hpp"

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
        for(auto &x : Board::zobrist_castling)
            x = rng();

        // fill en passant
        for(auto &x : Board::zobrist_en_passant_file)
            x = rng();

        // black to move
        Board::zobrist_black_to_move = rng();
    }
} _zInit;

Board::Board(const std::string& fen):
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
    mailbox[sq] = (as_int(c) << 3) | as_int(pt);

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
    int c = piece >> 3;
    int pt = piece & 0b111;

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
                int c = code >> 3;
                int pt = code & 0b111;
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
        char file_c = char('a' + (en_passant_square & 7));
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