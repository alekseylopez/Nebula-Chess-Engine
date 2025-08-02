#include "nebula/MagicBitboards.hpp"

#include <vector>

namespace nebula
{

std::array<uint64_t, 102400> MagicBitboards::rook_attacks;
std::array<uint64_t, 5248> MagicBitboards::bishop_attacks;

std::array<uint64_t*, 64> MagicBitboards::rook_attack_ptr;
std::array<uint64_t*, 64> MagicBitboards::bishop_attack_ptr;

std::array<uint64_t, 64> MagicBitboards::rook_masks;
std::array<uint64_t, 64> MagicBitboards::bishop_masks;

struct MagicInit
{
    MagicInit()
    {
        // initialize masks
        for(int sq = 0; sq < 64; ++sq)
        {
            MagicBitboards::rook_masks[sq] = MagicBitboards::generate_rook_mask(sq);
            MagicBitboards::bishop_masks[sq] = MagicBitboards::generate_bishop_mask(sq);
        }

        // initialize attack tables
        MagicBitboards::init_rook_attacks();
        MagicBitboards::init_bishop_attacks();
    }
} _magicInit;

uint64_t MagicBitboards::generate_rook_mask(int sq)
{
    uint64_t mask = 0ULL;
    int r = sq / 8;
    int f = sq % 8;

    // horizontal - exclude edges
    for(int i = f + 1; i < 7; ++i)
        mask |= 1ULL << (r * 8 + i);
    for(int i = f - 1; i > 0; --i)
        mask |= 1ULL << (r * 8 + i);

    // vertical - exclude edges
    for(int i = r + 1; i < 7; ++i)
        mask |= 1ULL << (i * 8 + f);
    for(int i = r - 1; i > 0; --i)
        mask |= 1ULL << (i * 8 + f);

    return mask;
}

uint64_t MagicBitboards::generate_bishop_mask(int sq)
{
    uint64_t mask = 0ULL;
    int r = sq / 8;
    int f = sq % 8;

    // four diagonal directions - exclude edges
    for(int i = 1; r + i < 7 && f + i < 7; ++i)
        mask |= 1ULL << ((r + i) * 8 + (f + i));
    for(int i = 1; r + i < 7 && f - i > 0; ++i)
        mask |= 1ULL << ((r + i) * 8 + (f - i));
    for(int i = 1; r - i > 0 && f + i < 7; ++i)
        mask |= 1ULL << ((r - i) * 8 + (f + i));
    for(int i = 1; r - i > 0 && f - i > 0; ++i)
        mask |= 1ULL << ((r - i) * 8 + (f - i));

    return mask;
}

uint64_t MagicBitboards::generate_rook_attacks_slow(int sq, uint64_t occ)
{
    uint64_t attacks = 0ULL;
    int r = sq / 8;
    int f = sq % 8;

    // rook directions
    const int dirs[4][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 } };

    for(auto [dr, df] : dirs)
    {
        for(int i = 1; i < 8; ++i)
        {
            int nr = r + i * dr;
            int nf = f + i * df;
            
            if(nr < 0 || nr >= 8 || nf < 0 || nf >= 8)
                break;
                
            int target_sq = nr * 8 + nf;
            attacks |= 1ULL << target_sq;
            
            if(occ & (1ULL << target_sq))
                break;
        }
    }

    return attacks;
}

uint64_t MagicBitboards::generate_bishop_attacks_slow(int sq, uint64_t occ)
{
    uint64_t attacks = 0ULL;
    int r = sq / 8;
    int f = sq % 8;

    // bishop directions
    const int dirs[4][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };

    for(auto [dr, df] : dirs)
    {
        for(int i = 1; i < 8; ++i)
        {
            int nr = r + i * dr;
            int nf = f + i * df;
            
            if(nr < 0 || nr >= 8 || nf < 0 || nf >= 8)
                break;
                
            int target_sq = nr * 8 + nf;
            attacks |= 1ULL << target_sq;
            
            if(occ & (1ULL << target_sq))
                break;
        }
    }

    return attacks;
}

void MagicBitboards::init_rook_attacks()
{
    int offset = 0;

    for(int sq = 0; sq < 64; ++sq)
    {
        rook_attack_ptr[sq] = &rook_attacks[offset];
        
        uint64_t mask = rook_masks[sq];
        int bits = __builtin_popcountll(mask);
        int permutations = 1 << bits;
        
        // generate all possible occupancy permutations
        std::vector<int> bit_indices;
        for(int i = 0; i < 64; ++i)
            if(mask & (1ULL << i))
                bit_indices.push_back(i);
        
        for(int i = 0; i < permutations; ++i)
        {
            uint64_t occ = 0ULL;
            for(int j = 0; j < bits; ++j)
                if(i & (1 << j))
                    occ |= 1ULL << bit_indices[j];
            
            uint64_t attacks = generate_rook_attacks_slow(sq, occ);
            
            // magic index
            uint64_t index = (occ * rook_magics[sq]) >> rook_shifts[sq];
            rook_attack_ptr[sq][index] = attacks;
        }
        
        offset += permutations;
    }
}

void MagicBitboards::init_bishop_attacks()
{
    int offset = 0;

    for(int sq = 0; sq < 64; ++sq)
    {
        bishop_attack_ptr[sq] = &bishop_attacks[offset];
        
        uint64_t mask = bishop_masks[sq];
        int bits = __builtin_popcountll(mask);
        int permutations = 1 << bits;
        
        // generate all possible occupancy permutations
        std::vector<int> bit_indices;
        for(int i = 0; i < 64; ++i)
            if(mask & (1ULL << i))
                bit_indices.push_back(i);
        
        for(int i = 0; i < permutations; ++i)
        {
            uint64_t occ = 0ULL;
            for(int j = 0; j < bits; ++j)
                if(i & (1 << j))
                    occ |= 1ULL << bit_indices[j];
            
            uint64_t attacks = generate_bishop_attacks_slow(sq, occ);
            
            // magic index
            uint64_t index = (occ * bishop_magics[sq]) >> bishop_shifts[sq];
            bishop_attack_ptr[sq][index] = attacks;
        }
        
        offset += permutations;
    }
}

}