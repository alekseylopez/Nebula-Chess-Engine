#ifndef NEBULA_DRIVER_HPP
#define NEBULA_DRIVER_HPP

#include "nebula/Board.hpp"

namespace nebula
{

// player vs. engine
void pve(Board& board, int depth, int max_moves);

// engine vs. engine
void eve(Board& board, int depth, int max_moves);

}

#endif