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
        // pawn
        {
            0,  0,  0,  0,   0,   0,  0,  0,  // rank 1
            5,  10, 10, -20, -20, 10, 10, 5,  // rank 2 - discourage staying home
            10, 10, 20, 30,  30,  20, 10, 10, // rank 3
            5,  5,  10, 50,  50,  10, 5,  5,  // rank 4 - strong central control
            0,  0,  0,  60,  60,  0,  0,  0,  // rank 5 - advanced central pawns
            5,  -5, -10, 20, 20, -10, -5, 5,  // rank 6
            5,  10, 10, -20, -20, 10, 10, 5,  // rank 7
            0,  0,  0,  0,   0,   0,  0,  0   // rank 8
        },

        // knight
        {
            -50, -40, -30, -30, -30, -30, -40, -50, // rank 1 - avoid back rank
            -40, -20, 0,   0,   0,   0,   -20, -40, // rank 2
            -30, 0,   10,  15,  15,  10,  0,   -30, // rank 3
            -30, 5,   15,  20,  20,  15,  5,   -30, // rank 4 - good central squares
            -30, 0,   15,  20,  20,  15,  0,   -30, // rank 5 - excellent outposts
            -30, 5,   10,  15,  15,  10,  5,   -30, // rank 6
            -40, -20, 0,   5,   5,   0,   -20, -40, // rank 7
            -50, -40, -30, -30, -30, -30, -40, -50  // rank 8 - avoid back rank
        },

        // bishop
        {
            -20, -10, -10, -10, -10, -10, -10, -20, // rank 1
            -10, 0,   0,   0,   0,   0,   0,   -10, // rank 2
            -10, 0,   5,   10,  10,  5,   0,   -10, // rank 3
            -10, 5,   5,   10,  10,  5,   5,   -10, // rank 4
            -10, 0,   10,  10,  10,  10,  0,   -10, // rank 5 - central activity
            -10, 10,  10,  10,  10,  10,  10,  -10, // rank 6 - good squares
            -10, 5,   0,   0,   0,   0,   5,   -10, // rank 7
            -20, -10, -10, -10, -10, -10, -10, -20  // rank 8
        },

        // rook
        {
            0,  0,  0,  0,  0,  0,  0,  0,  // rank 1 - starting position OK
            -5, 0,  0,  0,  0,  0,  0,  -5, // rank 2
            -5, 0,  0,  0,  0,  0,  0,  -5, // rank 3
            -5, 0,  0,  0,  0,  0,  0,  -5, // rank 4
            -5, 0,  0,  0,  0,  0,  0,  -5, // rank 5
            -5, 0,  0,  0,  0,  0,  0,  -5, // rank 6
            5,  10, 10, 10, 10, 10, 10, 5,  // rank 7 - 7th rank bonus
            0,  0,  0,  5,  5,  0,  0,  0   // rank 8
        },

        // queen
        {
            -20, -10, -10, -5, -5, -10, -10, -20, // rank 1 - starting position
            -10, 0,   0,   0,  0,  0,   0,   -10, // rank 2
            -10, 0,   5,   5,  5,  5,   0,   -10, // rank 3
            -5,  0,   5,   5,  5,  5,   0,   -5,  // rank 4
            0,   0,   5,   5,  5,  5,   0,   -5,  // rank 5
            -10, 5,   5,   5,  5,  5,   0,   -10, // rank 6
            -10, 0,   5,   0,  0,  0,   0,   -10, // rank 7
            -20, -10, -10, -5, -5, -10, -10, -20  // rank 8
        },

        // king
        {
            20,  30,  10,  0,   0,   10,  30,  20,  // rank 1 - safe back rank (castled position)
            20,  20,  0,   0,   0,   0,   20,  20,  // rank 2 - still relatively safe
            -10, -20, -20, -20, -20, -20, -20, -10, // rank 3 - getting dangerous
            -20, -30, -30, -40, -40, -30, -30, -20, // rank 4 - exposed
            -30, -40, -40, -50, -50, -40, -40, -30, // rank 5 - very exposed
            -30, -40, -40, -50, -50, -40, -40, -30, // rank 6 - dangerous
            -30, -40, -40, -50, -50, -40, -40, -30, // rank 7 - near enemy
            -30, -40, -40, -50, -50, -40, -40, -30  // rank 8 - very dangerous
        }
    };
};

}

#endif