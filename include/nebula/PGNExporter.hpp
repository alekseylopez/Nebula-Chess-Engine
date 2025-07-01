#ifndef NEBULA_PGNEXPORTER_HPP
#define NEBULA_PGNEXPORTER_HPP

#include "Board.hpp"

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

    inline std::string square(int sq) { return { static_cast<char>('a' + (sq % 8)), static_cast<char>('1' + (sq / 8)) }; }

    std::string to_san(const Move& m);

    std::string date();
};

}

#endif