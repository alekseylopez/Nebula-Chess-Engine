#include "nebula/Driver.hpp"
#include "nebula/Search.hpp"

namespace nebula
{

void pve(Board& board, int depth, int max_moves)
{
    board.print();

    Search engine(depth);

    for(int i = 0; i < max_moves; ++i)
    {
        if(i % 2 == 0) // player turn
        {
            std::string uci;

            std::cout << "Enter move in UCI format:\n";
            std::cin >> uci;

            Move move = board.from_uci(uci);

            std::vector<Move> legal = board.generate_moves();
            if(std::find(legal.begin(), legal.end(), move) == legal.end())
            {
                std::cout << "Not legal, try again!\n";
                --i;
                continue;
            }

            board.make_move(move);

            board.print();
        } else // engine turn
        {
            nebula::Move m;
            double eval;

            bool successful = engine.best_move(board, m, eval);

            if(successful)
            {
                std::cout << "Engine played " << m.uci() << '\n';

                board.make_move(m);

                board.print();
            } else
            {
                std::cout << "Coudln't generate moves.\n";

                break;
            }
        }
    }
}

void eve(Board& board, int depth, int max_moves)
{
    board.print();

    Search engine(depth);

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
}

}