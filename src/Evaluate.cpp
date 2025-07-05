#include "nebula/Evaluate.hpp"
#include "nebula/Values.hpp"

namespace nebula
{

int Evaluate::evaluate(const Board& board)
{
    int score = 0;

    // one calculation
    double phase = phase_of_game(board);
    
    score += material(board, phase);
    score += castling_bonus(board, phase);
    
    return (board.turn() == Color::White) ? score : -score;
}

int Evaluate::material(const Board& board, double phase)
{
    int mat = 0;
    int opening_score = 0;
    int endgame_score = 0;

    for(int piece = 0; piece < Board::num_piece_types; ++piece)
    {
        PieceType pt = static_cast<PieceType>(piece);
        
        // white pieces
        uint64_t white_pieces = board.pieces(Color::White, pt);
        while(white_pieces)
        {
            int sq = __builtin_ctzll(white_pieces);
            mat += Values::material_value[piece];
            opening_score += Values::pst[piece][sq];
            endgame_score += Values::pst_endgame[piece][sq];
            white_pieces &= white_pieces - 1;
        }
        
        // black pieces
        uint64_t black_pieces = board.pieces(Color::Black, pt);
        while(black_pieces)
        {
            int sq = __builtin_ctzll(black_pieces);
            mat -= Values::material_value[piece];
            // mirror PST scores vertically
            opening_score -= Values::pst[piece][sq ^ 56];
            endgame_score -= Values::pst[piece][sq ^ 56];
            black_pieces &= black_pieces - 1;
        }
    }

    int blended_pst = static_cast<int>(opening_score * phase + endgame_score * (1.0 - phase));

    return mat + blended_pst;
}

int Evaluate::castling_bonus(const Board& board, double phase)
{
    int bonus = 0;

    // castling rights bonus
    if(board.castling() & (board.castle_K | board.castle_Q))
        bonus += Values::castle_rights_bonus;
    if(board.castling() & (board.castle_k | board.castle_q))
        bonus -= Values::castle_rights_bonus;

    // detect castled position
    int wk = board.king_sq(Color::White);
    if(wk == 6 || wk == 2)
        bonus += Values::castled_position_bonus;
    int bk = board.king_sq(Color::Black);
    if(bk == 62 || bk == 58)
        bonus -= Values::castled_position_bonus;

    // blend by opening phase
    bonus = static_cast<int>(bonus * phase);

    return bonus;
}

}