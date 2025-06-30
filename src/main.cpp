#include "nebula/Board.hpp"
#include "nebula/Search.hpp"

#include <iostream>
#include <iomanip>

int main(int argc, char* argv[])
{
    std::cout << std::fixed << std::setprecision(2);

    if(argc > 4 || argc < 3)
    {
        std::cerr << "Usage:    " << argv[0] << " <depth> <max moves> [FEN]\nMake sure to include quotes around a FEN string.\n";
        return 1;
    }

    int depth = std::stoi(argv[1]);
    int max_moves = std::stoi(argv[2]);

    nebula::Board board = argc == 3 ? nebula::Board() : nebula::Board(std::string(argv[3]));

    board.print();

    nebula::Search engine(depth);

    for(int i = 0; i < max_moves; ++i)
    {
        nebula::Move m;
        double eval;

        bool successful = engine.best_move(board, m, eval);

        if(successful)
        {
            std::cout << eval << ": " << m.uci() << '\n';

            board.make_move(m);

            board.print();
        } else
        {
            std::cout << "Coudln't generate moves.\n";

            break;
        }
    }
    
    return 0;
}