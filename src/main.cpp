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
    int depth = 8;
    int max_moves = std::numeric_limits<int>::max();

    nebula::ReturnCode code = nebula::opts(argc, argv, mode, depth, max_moves);
    
    if(code == nebula::ReturnCode::Good)
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
    } else
    {
        if(code == nebula::ReturnCode::Help)
            return 0;
        
        if(code == nebula::ReturnCode::Error)
            return 1;
    }
    
    return 0;
}