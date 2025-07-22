#include "nebula/Driver.hpp"
#include "nebula/Search.hpp"
#include "nebula/PGNExporter.hpp"

namespace nebula
{

void pve(Board& board, int depth, int max_moves)
{
    board.print();

    Search engine(depth);
    PGNExporter wrapper(&board);

    for(int i = 0; i < max_moves; ++i)
    {
        if(i % 2 == 0) // player turn
        {
            std::vector<Move> legal = board.generate_moves();

            if(legal.empty())
            {
                if(board.in_check())
                {
                    std::cout << "Checkmate!\n\n";
                    wrapper.set_tag("Result", "0-1");
                } else
                {
                    std::cout << "Draw by stalemate.\n\n";
                    wrapper.set_tag("Result", "1/2-1/2");
                }
                
                break;
            }

            if(board.is_fifty_move_rule())
            {
                std::cout << "Draw by 50-move rule.\n\n";
                wrapper.set_tag("Result", "1/2-1/2");

                break;
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

            wrapper.make_move(move);

            board.print();

            if(board.is_repetition())
            {
                std::cout << "Draw by repetition.\n\n";
                wrapper.set_tag("Result", "1/2-1/2");

                break;
            }
        } else // engine turn
        {
            std::vector<Move> legal = board.generate_moves();

            if(legal.empty())
            {
                if(board.in_check())
                {
                    std::cout << "Checkmate!\n\n";
                    wrapper.set_tag("Result", "1-0");
                } else
                {
                    std::cout << "Draw by stalemate.\n\n";
                    wrapper.set_tag("Result", "1/2-1/2");
                }
                
                break;
            }

            if(board.is_fifty_move_rule())
            {
                std::cout << "Draw by 50-move rule.\n\n";
                wrapper.set_tag("Result", "1/2-1/2");
                
                break;
            }

            nebula::Move m;
            double eval;

            bool successful = engine.best_move(board, m, eval);

            if(successful)
            {
                std::cout << "Engine played " << m.uci() << '\n';

                wrapper.make_move(m);

                board.print();
            } else // shouldn't happen
            {
                std::cout << "Coudln't generate moves.\n";

                break;
            }
            
            if(board.is_repetition())
            {
                std::cout << "Draw by repetition.\n\n";
                wrapper.set_tag("Result", "1/2-1/2");

                break;
            }
        }
    }

    std::cout << wrapper.out();
}

void eve(Board& board, int depth, int max_moves)
{
    board.print();

    Search engine(depth);
    PGNExporter wrapper(&board);

    for(int i = 0; i < max_moves; ++i)
    {
        std::vector<Move> legal = board.generate_moves();

        if(legal.empty())
        {
            if(board.in_check())
            {
                std::cout << "Checkmate!\n\n";
                wrapper.set_tag("Result", i % 2 == 0 ? "0-1" : "1-0");
            } else
            {
                std::cout << "Draw by stalemate.\n\n";
                wrapper.set_tag("Result", "1/2-1/2");
            }
            
            break;
        }

        if(board.is_fifty_move_rule())
        {
            std::cout << "Draw by 50-move rule.\n\n";
            wrapper.set_tag("Result", "1/2-1/2");
            
            break;
        }

        nebula::Move m;
        double eval;

        bool successful = engine.best_move(board, m, eval);

        if(successful)
        {
            std::cout << eval << ": " << m.uci() << '\n';

            wrapper.make_move(m);

            board.print();
        } else // shouldn't happen
        {
            std::cout << "Coudln't generate moves.\n";

            break;
        }

        if(board.is_repetition())
        {
            std::cout << "Draw by repetition.\n\n";
            wrapper.set_tag("Result", "1/2-1/2");

            break;
        }
    }

    std::cout << wrapper.out();
}

}