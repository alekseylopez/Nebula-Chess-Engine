#include "nebula/Board.hpp"
#include "nebula/CLIHelper.hpp"
#include "nebula/Driver.hpp"

#include <iostream>
#include <iomanip>
#include <limits>

int main(int argc, char* argv[])
{
    std::cout << std::fixed << std::setprecision(2);

    // input
    nebula::InputMode mode;
    int depth = 6;
    int max_moves = std::numeric_limits<int>::max();

    switch(nebula::opts(argc, argv, mode, depth, max_moves))
    {
        case nebula::ReturnCode::Good:
        {
            nebula::Board board;

            switch(mode)
            {
                case nebula::InputMode::PlayerInput:
                    nebula::pve(board, depth, max_moves);
                    break;
                
                case nebula::InputMode::Auto:
                    nebula::eve(board, depth, max_moves);
                    break;
            }

            break;
        }

        case nebula::ReturnCode::Help:
            return 0;
            break;
        
        case nebula::ReturnCode::Error:
            return 1;
            break;
    }
    
    return 0;
}