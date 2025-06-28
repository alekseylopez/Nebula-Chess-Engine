#include "nebula/Board.hpp"
#include "nebula/Search.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    if(argc > 4 || argc < 3)
    {
        std::cerr << "Usage:    " << argv[0] << " <depth> <max moves> [FEN]\nMake sure to include quotes around a FEN string.\n";
        return 1;
    }

    int depth = std::stoi(argv[1]);
    int max_moves = std::stoi(argv[2]);

    nebula::Board board = argc == 3 ? nebula::Board() : nebula::Board(std::string(argv[3]));

    board.print();

    for(int i = 0; i < max_moves; ++i)
    {
        nebula::Move m;
        nebula::SearchResult flag;
        double eval = nebula::search_root(board, depth, m, flag);

        if(flag == nebula::SearchResult::Checkmate)
        {
            std::cout << "Checkmate!\n\n";
            break;
        } else if(flag == nebula::SearchResult::Stalemate)
        {
            std::cout << "Stalemate!\n\n";
            break;
        }

        std::cout << eval << ": " << m.uci() << '\n';

        board.make_move(m);

        board.print();
    }
    
    return 0;
}