#ifndef NEBULA_VALUES_HPP
#define NEBULA_VALUES_HPP

namespace nebula
{

struct Values
{
    static constexpr int material_value[6] =
    {
        100, // Pawn
        320, // Knight
        330, // Bishop
        500, // Rook
        900, // Queen
        0    // King
    };

    static constexpr int pst[6][64] =
    {
        // pawn - encourage central control, discourage staying back too long
        {
              0,   0,   0,   0,   0,   0,   0,   0,  // rank 1
              5,  10,  10, -20, -20,  10,  10,   5,  // rank 2 - mild penalty for staying home
             10,  10,  20,  30,  30,  20,  10,  10,  // rank 3 - developed pawns
             20,  20,  30,  40,  40,  30,  20,  20,  // rank 4 - strong central control
             30,  30,  30,  50,  50,  30,  30,  30,  // rank 5 - advanced pawns
             40,  40,  20,  30,  30,  20,  40,  40,  // rank 6 - passed pawn potential
             50,  50,  50,  50,  50,  50,  50,  50,  // rank 7 - near promotion
              0,   0,   0,   0,   0,   0,   0,   0   // rank 8
        },

        // knight - encourage development and centralization
        {
            -50, -40, -30, -30, -30, -30, -40, -50,  // rank 1 - mild back rank penalty
            -40, -20,   0,   5,   5,   0, -20, -40,  // rank 2 - can develop from here
            -30,   5,  10,  15,  15,  10,   5, -30,  // rank 3 - good development squares
            -30,  10,  15,  20,  20,  15,  10, -30,  // rank 4 - excellent central squares
            -30,  10,  15,  20,  20,  15,  10, -30,  // rank 5 - great outposts
            -30,   5,  10,  15,  15,  10,   5, -30,  // rank 6 - still active
            -40, -20,   0,   0,   0,   0, -20, -40,  // rank 7 - less ideal
            -50, -40, -30, -30, -30, -30, -40, -50   // rank 8 - avoid back rank
        },

        // bishop - reward activity and long diagonals
        {
            -20, -10, -10, -10, -10, -10, -10, -20,  // rank 1 - mild penalty
            -10,   5,   0,   0,   0,   0,   5, -10,  // rank 2 - can develop
            -10,  10,  10,  10,  10,  10,  10, -10,  // rank 3 - good development
            -10,   0,  10,  10,  10,  10,   0, -10,  // rank 4 - active squares
            -10,   5,   5,  10,  10,   5,   5, -10,  // rank 5 - central activity
            -10,   0,   5,  10,  10,   5,   0, -10,  // rank 6 - still good
            -10,   0,   0,   0,   0,   0,   0, -10,  // rank 7 - less ideal
            -20, -10, -10, -10, -10, -10, -10, -20   // rank 8 - avoid back rank
        },

        // rook - reward open files and 7th rank
        {
               0,   0,   5,  10,  10,   5,   0,   0,  // rank 1 - starting position OK
               0,   0,   0,   0,   0,   0,   0,   0,  // rank 2 - neutral
               0,   0,   0,   0,   0,   0,   0,   0,  // rank 3 - neutral
               0,   0,   0,   0,   0,   0,   0,   0,  // rank 4 - neutral
               0,   0,   0,   0,   0,   0,   0,   0,  // rank 5 - neutral
               0,   0,   0,   0,   0,   0,   0,   0,  // rank 6 - neutral
              20,  20,  20,  20,  20,  20,  20,  20,  // rank 7 - 7th rank bonus
               0,   0,   0,   0,   0,   0,   0,   0   // rank 8 - neutral
        },

        // queen - slight penalty for early development, reward central activity
        {
            -20, -10, -10,  -5,  -5, -10, -10, -20,  // rank 1 - starting position
            -10,   0,   5,   0,   0,   0,   0, -10,  // rank 2 - don't develop too early
            -10,   5,   5,   5,   5,   5,   0, -10,  // rank 3 - OK if needed
             -5,   0,   5,   5,   5,   5,   0,  -5,  // rank 4 - decent squares
             -5,   0,   5,   5,   5,   5,   0,  -5,  // rank 5 - active squares
            -10,   0,   5,   5,   5,   5,   0, -10,  // rank 6 - still active
            -10,   0,   0,   0,   0,   0,   0, -10,  // rank 7 - less ideal
            -20, -10, -10,  -5,  -5, -10, -10, -20   // rank 8 - avoid back rank
        },

        // king - encourage castling, discourage exposure
        {
              30,  40,  10,   0,   0,  10,  40,  30,  // rank 1 - castled position good
              20,  20,   0,   0,   0,   0,  20,  20,  // rank 2 - still safe
             -10, -20, -20, -20, -20, -20, -20, -10,  // rank 3 - getting dangerous
             -20, -30, -30, -40, -40, -30, -30, -20,  // rank 4 - exposed
             -30, -40, -40, -50, -50, -40, -40, -30,  // rank 5 - very exposed
             -30, -40, -40, -50, -50, -40, -40, -30,  // rank 6 - dangerous
             -30, -40, -40, -50, -50, -40, -40, -30,  // rank 7 - near enemy
             -30, -40, -40, -50, -50, -40, -40, -30   // rank 8 - very dangerous
        }
    };
};

}

#endif