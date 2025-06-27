#include "nebula/Board.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    nebula::Board* board;

    if(argc == 1)
    {
        board = new nebula::Board();
    } else if(argc == 2)
    {
        board = new nebula::Board(std::string(argv[1]));
    } else
    {
        std::cerr << "Usage:\n    " << argv[0] << "\nor\n    " << argv[0] << " \"<FEN>\"\n";
        return 1;
    }

    board->print();
    
    return 0;
}