#ifndef NEBULA_PGNEXPORTER_HPP
#define NEBULA_PGNEXPORTER_HPP

#include "nebula/Board.hpp"

namespace nebula
{

class PGNExporter
{
public:
    explicit PGNExporter(Board* board);

    // set tags
    void set_tag(const std::string& key, const std::string& value);

    // also makes the move
    void make_move(const Move& move);

    // get the PGN
    std::string out();
private:
    std::vector<std::pair<std::string,std::string>> tags;
    std::vector<std::string> moves;
    Board* board;

    // gets algebraic notation for square
    inline std::string square(int sq) { return { static_cast<char>('a' + (sq % 8)), static_cast<char>('1' + (sq / 8)) }; }

    // makes a move and turns it into algebraic notation
    std::string to_san(const Move& m);

    // gets today's date
    std::string date();
};

}

#endif