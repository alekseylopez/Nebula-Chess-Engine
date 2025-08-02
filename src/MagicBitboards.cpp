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

}