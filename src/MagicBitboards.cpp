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

}