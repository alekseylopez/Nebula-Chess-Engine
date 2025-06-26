#include "nebula/Board.hpp"

#include <random>
#include <sstream>
#include <stdexcept>
#include <cctype>

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

Board::Board():
    Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {}

Board::Board(const std::string& fen):
    pieces_bb{{{0ULL}}},
    color_bb{{0ULL}},
    all_pieces_bb{0ULL},
    side_to_move{Color::White},
    castling_rights{0},
    en_passant_square{-1},
    half_moves{0},
    full_move{1},
    zobrist_key{0ULL}
{
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

uint64_t Board::pieces(Color c, PieceType pt) const
{
    return pieces_bb[as_int(c)][as_int(pt)];
}

uint64_t Board::occupancy(Color c) const
{
    return color_bb[as_int(c)];
}

uint64_t Board::occupancy() const
{
    return all_pieces_bb;
}

int Board::piece_at(int sq) const
{
    // if invalid return -1
    if(sq < 0 || sq >= 64)
        return -1;

    // translate square number to mask
    const uint64_t mask = 1ULL << sq;

    // if empty return -1
    if(!(all_pieces_bb & mask))
        return -1;
    
    // find what the piece is
    for(int c = 0; c < num_colors; ++c)
        if(color_bb[c] & mask)
            for(int pt = 0; pt < num_piece_types; ++pt)
                if(pieces_bb[c][pt] & mask)
                    return c * num_piece_types + pt;
    
    // should never get here
    return -1;
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

    // place the piece;
    pieces_bb[as_int(c)][as_int(pt)] |= mask;
    color_bb[as_int(c)] |= mask;
    all_pieces_bb |= mask;

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
    int c = piece / num_piece_types;
    int pt = piece % num_piece_types;

    // remove that piece
    pieces_bb[c][pt] &= ~mask;
    color_bb[c] &= ~mask;
    all_pieces_bb &= ~mask;

    // update Zobrist key
    update_zobrist_piece(sq, as_color(c), as_piece_type(pt));
}

int Board::piece_char_to_code(char c, Color& out_c, PieceType& out_pt) const
{
    if(std::isupper(static_cast<unsigned char>(c)))
        out_c = Color::White;
    else if(std::islower(static_cast<unsigned char>(c)))
        out_c = Color::Black;
    else
        return -1;

    char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

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

    return 0;
}

}