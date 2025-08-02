#ifndef NEBULA_MAGICBITBOARDS_HPP
#define NEBULA_MAGICBITBOARDS_HPP

#include <array>
#include <cstdint>

namespace nebula
{

struct MagicBitboards
{
public:
    static constexpr uint64_t rook_magics[64] =
    {
        0x8000825520c000ULL,    0x40200040001006ULL,   0x828010008020000cULL,  0x480048008001000ULL,
        0x200081020040200ULL,   0x4600041002000801ULL, 0x40000a118023004ULL,   0x200040025025082ULL,
        0xc00800080204000ULL,   0x101003080400100ULL,  0x40a1001100402004ULL,  0x3602002008104200ULL,
        0x8000808004000800ULL,  0x2049004400030008ULL, 0x4040800200800100ULL,  0xcc01002100008052ULL,
        0xd40008000802045ULL,   0x2004888020004000ULL, 0x6040110020010040ULL,  0x4000090020100300ULL,
        0x8361828004000800ULL,  0x402808002000400ULL,  0x200440090013208ULL,   0x8223200050080ccULL,
        0x8080208000400aULL,    0x2802010600208848ULL, 0x2260080040100040ULL,  0x1008100080080086ULL,
        0xa140080080040082ULL,  0x9042000200081004ULL, 0xc202004040800100ULL,  0x8000808200004401ULL,
        0xf00400020800082ULL,   0x2510002000404009ULL, 0x4420200181801000ULL,  0x1420e10009005000ULL,
        0x200800400800801ULL,   0xa0800200800401ULL,   0x4001b00c00081aULL,    0x490008106000444ULL,
        0x40400020808000ULL,    0x6410002000404000ULL, 0x10108200420020ULL,    0x1619a3001001000aULL,
        0x102a0010600a0004ULL,  0xc1002400090012ULL,   0x2800104609840018ULL,  0x18218844060001ULL,
        0x20208000400180ULL,    0x400080200280ULL,     0x108021460600ULL,      0x4000080010008280ULL,
        0x6080080004008080ULL,  0x8a000410080200ULL,   0x8150080210010400ULL,  0x8080004421088200ULL,
        0x2821001020800049ULL,  0x806a40011101ULL,     0x8820400a001082ULL,    0xd442004008042012ULL,
        0x1202000410210802ULL,  0x802005001840802ULL,  0x210011000880204ULL,   0x1000104408c0422ULL
    };

    static constexpr uint64_t bishop_magics[64] =
    {
        0x80c0010408820048ULL,  0x248014800810041ULL,  0x8082100280100ULL,     0x40220a0202006000ULL,
        0x4504010290000ULL,     0xc000823040101812ULL, 0x400c210b00382ULL,     0x42a210105400ULL,
        0x2080411104010045ULL,  0x8000104112040441ULL, 0x895100400405200ULL,   0x1040425880090ULL,
        0x4004020210012014ULL,  0xbc000822022100bULL,  0x18208052c1084ULL,     0x1020105218040208ULL,
        0x908201021012400ULL,   0x204092108008106ULL,  0x2188011020401022ULL,  0x8068222004020ULL,
        0x402004400a20000ULL,   0x380a000022102200ULL, 0x201000201104230ULL,   0x202060080410820ULL,
        0x2010400308089900ULL,  0x18200005040088ULL,   0x400504028080040ULL,   0x90040020440008ULL,
        0x20100102d004004ULL,   0x4810002005a00ULL,    0xa11022001080158ULL,   0x8005002017108803ULL,
        0x310402080021a001ULL,  0x2804040306245020ULL, 0x1006002200101084ULL,  0x8200840400080210ULL,
        0x10200820220020ULL,    0x4081001000020a1ULL,  0x4008820080104810ULL,  0x28088822248a10ULL,
        0x4028230104101ULL,     0x48482208181004ULL,   0x8c200444c1000ULL,     0x20212000400ULL,
        0x1020081010400400ULL,  0x200402a0202e02ULL,   0x2011040920a080ULL,    0x1042400408890ULL,
        0x80010808024a1000ULL,  0x28018048484c0000ULL, 0xc140008068080041ULL,  0x820d41042022000ULL,
        0x480400620820000ULL,   0x20400204010480ULL,   0x8004302421440000ULL,  0x806096020204a220ULL,
        0x2010400440208c1ULL,   0x10600303e1042000ULL, 0x104908493000ULL,      0x48010800c20a00ULL,
        0x40080000c0028210ULL,  0xa000ae0082080ULL,    0x200840d042008914ULL,  0x8210211208840100ULL
    };

    static constexpr int rook_shifts[64] =
    {
        52, 53, 53, 53, 53, 53, 53, 52,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        52, 53, 53, 53, 53, 53, 53, 52
    };

    static constexpr int bishop_shifts[64] =
    {
        58, 59, 59, 59, 59, 59, 59, 58,
        59, 59, 59, 59, 59, 59, 59, 59,
        59, 59, 57, 57, 57, 57, 59, 59,
        59, 59, 57, 55, 55, 57, 59, 59,
        59, 59, 57, 55, 55, 57, 59, 59,
        59, 59, 57, 57, 57, 57, 59, 59,
        59, 59, 59, 59, 59, 59, 59, 59,
        58, 59, 59, 59, 59, 59, 59, 58
    };

    // attack tables
    static std::array<uint64_t, 102400> rook_attacks;
    static std::array<uint64_t, 5248> bishop_attacks;
    
    // offsets into attack tables
    static std::array<uint64_t*, 64> rook_attack_ptr;
    static std::array<uint64_t*, 64> bishop_attack_ptr;

    // mask generation
    static std::array<uint64_t, 64> rook_masks;
    static std::array<uint64_t, 64> bishop_masks;

    // get rook attacks for a square with given occupancy
    static inline uint64_t get_rook_attacks(int sq, uint64_t occ)
    {
        occ &= rook_masks[sq];
        occ *= rook_magics[sq];
        occ >>= rook_shifts[sq];

        return rook_attack_ptr[sq][occ];
    }

    // get bishop attacks for a square with given occupancy
    static inline uint64_t get_bishop_attacks(int sq, uint64_t occ)
    {
        occ &= bishop_masks[sq];
        occ *= bishop_magics[sq];
        occ >>= bishop_shifts[sq];

        return bishop_attack_ptr[sq][occ];
    }

    // get queen attacks
    static inline uint64_t get_queen_attacks(int sq, uint64_t occ)
    {
        return get_rook_attacks(sq, occ) | get_bishop_attacks(sq, occ);
    }

private:
    static uint64_t generate_rook_mask(int sq);
    static uint64_t generate_bishop_mask(int sq);
    static uint64_t generate_rook_attacks_slow(int sq, uint64_t occ);
    static uint64_t generate_bishop_attacks_slow(int sq, uint64_t occ);
    static void init_rook_attacks();
    static void init_bishop_attacks();

    friend struct MagicInit;
};

}

#endif