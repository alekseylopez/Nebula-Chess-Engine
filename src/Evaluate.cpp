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
    score += pawn_structure(board, phase);
    
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

int Evaluate::pawn_structure(const Board& board, double phase)
{
    int score = 0;
    
    // weaknesses for each color
    score += analyze_pawn_weaknesses(board, Color::White, phase);
    score -= analyze_pawn_weaknesses(board, Color::Black, phase);
    
    // passed pawns for each color
    score += analyze_passed_pawns(board, Color::White, phase);
    score -= analyze_passed_pawns(board, Color::Black, phase);
    
    return score;
}

int Evaluate::analyze_pawn_weaknesses(const Board& board, Color color, double phase)
{
    int penalty = 0;
    uint64_t pawns = board.pieces(color, PieceType::Pawn);
    
    // pawns per file
    std::array<int, 8> file_counts = { 0 };
    uint64_t temp_pawns = pawns;

    while(temp_pawns)
    {
        int sq = __builtin_ctzll(temp_pawns);

        ++file_counts[sq & 7];

        temp_pawns &= temp_pawns - 1;
    }
    
    // score for each pawn
    temp_pawns = pawns;
    while(temp_pawns)
    {
        int sq = __builtin_ctzll(temp_pawns);

        int file = sq & 7;
        
        // isolated pawn penalty
        if(is_isolated_pawn(board, color, file))
        {
            int isolated_penalty = Values::isolated_pawn_penalty;

            // even worse in endgame
            isolated_penalty = static_cast<int>(isolated_penalty * (1.0 + (1.0 - phase) * 0.5));
            
            penalty += isolated_penalty;
        }
        
        // doubled pawn penalty
        if(file_counts[file] > 1)
            penalty += Values::doubled_pawn_penalty;
        
        // backward pawn penalty
        if(is_backward_pawn(board, color, sq))
            penalty += Values::backward_pawn_penalty;
        
        temp_pawns &= temp_pawns - 1;
    }
    
    return -penalty; //  these are penalties
}

int Evaluate::analyze_passed_pawns(const Board& board, Color color, double phase)
{
    int bonus = 0;
    uint64_t pawns = board.pieces(color, PieceType::Pawn);
    
    while(pawns)
    {
        int sq = __builtin_ctzll(pawns);
        
        if(is_passed_pawn(board, color, sq)) {
            int rank = (color == Color::White) ? (sq >> 3) : (7 - (sq >> 3));
            bonus += passed_pawn_value(rank, phase);
            
            // additional bonuses for advanced passed pawns
            if(rank >= 5)
            {
                // check if pawn is connected to other pawns
                uint64_t adjacent_files = 0;
                int file = sq & 7;
                if(file > 0)
                    adjacent_files |= board.pieces(color, PieceType::Pawn) & (0x0101010101010101ULL << (file - 1));
                if(file < 7)
                    adjacent_files |= board.pieces(color, PieceType::Pawn) & (0x0101010101010101ULL << (file + 1));
                
                if(adjacent_files)
                    bonus += Values::connected_passed_pawn_bonus;
                
                // check if pawn is protected by another pawn
                uint64_t protection_mask = (color == Color::White) ? ((1ULL << (sq - 7)) | (1ULL << (sq - 9))) : ((1ULL << (sq + 7)) | (1ULL << (sq + 9)));
                
                if(board.pieces(color, PieceType::Pawn) & protection_mask)
                    bonus += Values::protected_passed_pawn_bonus;
            }
        }
        
        pawns &= pawns - 1;
    }
    
    return bonus;
}

bool Evaluate::is_isolated_pawn(const Board& board, Color color, int file)
{
    // check adjacent files for friendly pawns
    uint64_t adjacent_files = 0;
    if(file > 0)
        adjacent_files |= board.pieces(color, PieceType::Pawn) & (0x0101010101010101ULL << (file - 1));
    if(file < 7)
        adjacent_files |= board.pieces(color, PieceType::Pawn) & (0x0101010101010101ULL << (file + 1));
    
    return adjacent_files == 0;
}

bool Evaluate::is_doubled_pawn(const Board& board, Color color, int file)
{
    uint64_t file_pawns = board.pieces(color, PieceType::Pawn) & (0x0101010101010101ULL << file);
    
    return __builtin_popcountll(file_pawns) > 1;
}

bool Evaluate::is_backward_pawn(const Board& board, Color color, int square)
{
    int file = square & 7;
    int rank = square >> 3;
    
    // check if pawn can be supported by adjacent pawns
    bool can_be_supported = false;
    
    // left file
    if(file > 0)
    {
        uint64_t left_file_pawns = board.pieces(color, PieceType::Pawn) & (0x0101010101010101ULL << (file - 1));
        while(left_file_pawns)
        {
            int pawn_sq = __builtin_ctzll(left_file_pawns);

            int pawn_rank = pawn_sq >> 3;
            
            if(color == Color::White && pawn_rank <= rank)
                can_be_supported = true;
            if(color == Color::Black && pawn_rank >= rank)
                can_be_supported = true;
            
            left_file_pawns &= left_file_pawns - 1;
        }
    }
    
    // right file
    if(file < 7)
    {
        uint64_t right_file_pawns = board.pieces(color, PieceType::Pawn) & (0x0101010101010101ULL << (file + 1));
        while(right_file_pawns)
        {
            int pawn_sq = __builtin_ctzll(right_file_pawns);

            int pawn_rank = pawn_sq >> 3;
            
            if(color == Color::White && pawn_rank <= rank)
                can_be_supported = true;
            if(color == Color::Black && pawn_rank >= rank)
                can_be_supported = true;
            
            right_file_pawns &= right_file_pawns - 1;
        }
    }
    
    // check if square in front is controlled by enemy pawn
    bool blocked_by_enemy = false;
    int front_sq = (color == Color::White) ? square + 8 : square - 8;
    
    if(front_sq >= 0 && front_sq < 64)
    {
        uint64_t enemy_pawn_attacks = 0;
        if(color == Color::White)
        {
            // check for black pawn attacks
            if((front_sq & 7) > 0)
                enemy_pawn_attacks |= (1ULL << (front_sq + 7));
            if((front_sq & 7) < 7)
                enemy_pawn_attacks |= (1ULL << (front_sq + 9));
        } else
        {
            // check for white pawn attacks
            if((front_sq & 7) > 0)
                enemy_pawn_attacks |= (1ULL << (front_sq - 9));
            if((front_sq & 7) < 7)
                enemy_pawn_attacks |= (1ULL << (front_sq - 7));
        }
        
        blocked_by_enemy = (board.pieces(color == Color::White ? Color::Black : Color::White, PieceType::Pawn) & enemy_pawn_attacks) != 0;
    }
    
    return !can_be_supported && blocked_by_enemy;
}

bool Evaluate::is_passed_pawn(const Board& board, Color color, int square)
{
    int file = square & 7;
    int rank = square >> 3;
    
    // passed pawn zone: file and adjacent files ahead of the pawn
    uint64_t passed_zone = 0;
    
    if(color == Color::White)
    {
        // check ranks ahead
        for(int r = rank + 1; r < 8; r++)
        {
            if(file > 0)
                passed_zone |= (1ULL << (r * 8 + file - 1));

            passed_zone |= (1ULL << (r * 8 + file));

            if(file < 7)
                passed_zone |= (1ULL << (r * 8 + file + 1));
        }
    } else
    {
        // check ranks ahead
        for(int r = rank - 1; r >= 0; r--)
        {
            if(file > 0)
                passed_zone |= (1ULL << (r * 8 + file - 1));

            passed_zone |= (1ULL << (r * 8 + file));
            
            if(file < 7)
                passed_zone |= (1ULL << (r * 8 + file + 1));
        }
    }
    
    // check if any enemy pawns are in the passed zone
    uint64_t enemy_pawns = board.pieces(color == Color::White ? Color::Black : Color::White, PieceType::Pawn);
    
    return (enemy_pawns & passed_zone) == 0;
}

int Evaluate::passed_pawn_value(int rank, double phase)
{
    int base_value = Values::base_values[rank];
    
    // more valuable in endgame
    double endgame_multiplier = 1.0 + (1.0 - phase) * 1.5;
    
    return static_cast<int>(base_value * endgame_multiplier);
}

}