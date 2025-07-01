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
            std::vector<Move> legal = board.generate_moves();

            if(legal.empty())
            {
                if(board.in_check())
                    std::cout << "Checkmate!\n";
                else
                    std::cout << "Stalemate.\n";
                
                return;
            }

            std::string uci;

            std::cout << "Enter move in UCI format:\n";
            std::cin >> uci;

            Move move = board.from_uci(uci);
            if(std::find(legal.begin(), legal.end(), move) == legal.end())
            {
                std::cout << "Not legal, try again!\n";
                --i;
                continue;
            }

            board.make_move(move);

            board.print();

            if(board.is_repetition())
            {
                std::cout << "Draw by repetition.\n";

                return;
            }
        } else // engine turn
        {
            std::vector<Move> legal = board.generate_moves();

            if(legal.empty())
            {
                if(board.in_check())
                    std::cout << "Checkmate!\n";
                else
                    std::cout << "Stalemate!\n";
                
                return;
            }

            nebula::Move m;
            double eval;

            bool successful = engine.best_move(board, m, eval);

            if(successful)
            {
                std::cout << "Engine played " << m.uci() << '\n';

                board.make_move(m);

                board.print();
            } else // shouldn't happen
            {
                std::cout << "Coudln't generate moves.\n";

                return;
            }
            
            if(board.is_repetition())
            {
                std::cout << "Draw by repetition.\n";

                return;
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
        std::vector<Move> legal = board.generate_moves();

        if(legal.empty())
        {
            if(board.in_check())
                std::cout << "Checkmate!\n";
            else
                std::cout << "Stalemate!\n";
            
            return;
        }

        nebula::Move m;
        double eval;

        bool successful = engine.best_move(board, m, eval);

        if(successful)
        {
            std::cout << eval << ": " << m.uci() << '\n';

            board.make_move(m);

            board.print();
        } else // shouldn't happen
        {
            std::cout << "Coudln't generate moves.\n";

            break;
        }

        if(board.is_repetition())
        {
            std::cout << "Draw by repetition.\n";

            return;
        }
    }
}

}