#include "nebula/CLIHelper.hpp"

#include <iostream>
#include <getopt.h>

namespace nebula
{

ReturnCode opts(int argc, char* argv[], InputMode& mode, int& depth, int& length)
{
    opterr = static_cast<int>(false);

    const char* shortOptions = "hm:d:l:";

    option longOptions[] =
    {
        { "help", no_argument, nullptr, 'h' },
        { "mode", required_argument, nullptr, 'm' },
        { "depth", required_argument, nullptr, 'd' },
        { "length", required_argument, nullptr, 'l' },
        { nullptr, 0, nullptr, '\0' }
    };

    bool mChosen = false;

    int choice = 0;
    int index = 0;

    while((choice = getopt_long(argc, argv, shortOptions, longOptions, &index)) != -1)
    {
        switch(choice)
        {
            case 'h':
                std::cout << R"(
Usage: ./nebula -m MODE [OPTIONS]

Required:
-m, --mode MODE
        Specify input mode (required):
        PVE    Player vs. Engine (you enter moves)
        EVE    Engine vs. Engine (auto play)

Options:
-h, --help
        Show this help message and exit.

-d, --depth DEPTH
        Maximum search depth (positive integer).
        Default is set to 5.

-l, --length LENGTH
        Maximum game length in moves (positive integer).
        Default is unlimited.

Examples:
./nebula -m PVE --depth 6
./nebula --mode EVE -d 8 -l 200

)";

                return ReturnCode::Help;
                break;
            
            case 'm':
                if(std::string(optarg) == "PVE")
                {
                    mode = InputMode::PlayerInput;
                } else if(std::string(optarg) == "EVE")
                {
                    mode = InputMode::Auto;
                } else
                {
                    std::cerr << "Invalid mode; try ./nebula --help\n";
                    return ReturnCode::Error;
                }
                mChosen = true;
                break;
            
            case 'd':
                depth = std::stoi(optarg);
                break;
            
            case 'l':
                length = std::stoi(optarg);
                break;
            
            default:
                std::cerr << "Invalid command line optio; try ./nebula --helpn\n";
                return ReturnCode::Error;
        }
    }

    if(!mChosen)
    {
        std::cerr << "No mode specified; try ./nebula --help\n";
        return ReturnCode::Error;
    }

    return ReturnCode::Good;
}

}