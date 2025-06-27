#ifndef NEBULA_ATTACKTABLES_HPP
#define NEBULA_ATTACKTABLES_HPP

#include <array>
#include <cstdint>

namespace nebula
{

struct AttackTables
{
    static std::array<uint64_t, 64> knight;
    static std::array<uint64_t, 64> king;
    static std::array<std::array<uint64_t, 64>, 2> pawn;

    static constexpr std::array<std::pair<int, int>, 4> rook_dirs = { { { 0,  1 }, { 0, -1 }, { 1,  0 }, { -1,  0 } } };
    static constexpr std::array<std::pair<int, int>, 4> bishop_dirs = { { { 1,  1 }, { -1,  1 }, { -1, -1 }, { 1, -1 } } };
    static constexpr std::array<std::pair<int, int>, 8> queen_dirs = { { { 0,  1 }, { 0, -1 }, { 1,  0 }, { -1,  0 }, { 1,  1 }, { -1,  1 }, { -1, -1 }, { 1, -1 } } };
};

}

#endif