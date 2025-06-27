#include "nebula/AttackTables.hpp"

namespace nebula
{

std::array<uint64_t, 64> AttackTables::knight;
std::array<uint64_t, 64> AttackTables::king;
std::array<std::array<uint64_t, 64>, 2> AttackTables::pawn;

struct TablesInit
{
    static constexpr int knight_dirs[8][2] =
    {
        { 2, 1 }, { 1, 2 }, { -1, 2 }, { -2, 1 }, { -2, -1 }, { -1, -2 }, { 1, -2 }, { 2, -1 }
    };

    static constexpr int king_dirs[8][2] =
    {
        { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }
    };

    static constexpr int pawn_dirs[2][2][2] =
    {
        { { 1, -1 }, { 1, 1 } },
        { { -1, -1 }, { -1, 1 } }
    };

    TablesInit()
    {
        for(int sq = 0; sq < 64; ++sq)
        {
            int r = sq / 8;
            int f = sq % 8;

            // knight
            uint64_t km = 0ULL;
            for(const auto& d : knight_dirs)
            {
                int r2 = r + d[0];
                int f2 = f + d[1];

                if(r2 >= 0 && r2 < 8 && f2 >= 0 && f2 < 8)
                    km |= (1ULL << (r2 * 8 + f2));
            }
            AttackTables::knight[sq] = km;

            // king
            uint64_t km2 = 0ULL;
            for(const auto& d : king_dirs)
            {
                int r2 = r + d[0];
                int f2 = f + d[1];

                if(r2 >= 0 && r2 < 8 && f2 >= 0 && f2 < 8)
                    km2 |= (1ULL << (r2 * 8 + f2));
            }
            AttackTables::king[sq] = km2;

            // pawn
            for(int c = 0; c < 2; ++c)
            {
                uint64_t pm = 0ULL;
                for(const auto& d : pawn_dirs[c])
                {
                    int r2 = r + d[0];
                    int f2 = f + d[1];

                    if(r2 >= 0 && r2 < 8 && f2 >= 0 && f2 < 8)
                        pm |= (1ULL << (r2 * 8 + f2));
                }
                AttackTables::pawn[c][sq] = pm;
            }
        }
    }
} _tInit;

}