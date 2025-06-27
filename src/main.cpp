#include "nebula/Board.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    if(argc > 2)
    {
        std::cerr << "Usage:    " << argv[0] << " [FEN]\nMake sure to include quotes around a FEN string.\n";
        return 1;
    }

    nebula::Board board = argc == 1 ? nebula::Board() : nebula::Board(std::string(argv[1]));

    board.print();
    
    return 0;
}