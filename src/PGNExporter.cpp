#include "nebula/PGNExporter.hpp"

#include <sstream>

namespace nebula
{

PGNExporter::PGNExporter(Board* board):
    board(board)
{
    tags =
    {
        { "Event", "?" },
        { "Site", "?" },
        { "Date", date() },
        { "Round", "?" },
        { "White", "?" },
        { "Black", "?" },
        { "Result", "*" }
    };
}

void PGNExporter::set_tag(const std::string& key, const std::string& value)
{
    for(auto& [k, v] : tags)
    {
        if(k == key)
        {
            v = value;
            return;
        }
    }

    tags.emplace_back(key, value);
}

void PGNExporter::make_move(const Move& move)
{
    std::string san = to_san(move);

    if(board->turn() == Color::Black)
    {
        moves.push_back(std::to_string(board->full()) + ".");
        moves.push_back(san);
    } else
    {
        moves.push_back(san);
    }
}

std::string PGNExporter::out()
{
    std::ostringstream oss;

    for(const auto& [key, value] : tags)
        oss << '[' << key << " \"" << value << "\"]\n";
    
    oss << '\n';

    for(size_t i = 0; i < moves.size(); ++i)
        oss << moves[i] << (i + 1 < moves.size() ? " " : "") << (i % 15 == 14 && i < moves.size() - 1 ? "\n" : "");
    
    oss << ' ' << tags[6].second << "\n\n";
    
    return oss.str();
}

std::string PGNExporter::to_san(const Move& move)
{
    std::string s;

    // castling is easy
    if(move.flags & static_cast<uint8_t>(MoveFlag::KingCastle))
    {
        s += "O-O";
    } else if(move.flags & static_cast<uint8_t>(MoveFlag::QueenCastle))
    {
        s += "O-O-O";
    } else
    {
        int pt = move.piece & 0b111;
        char p = std::toupper(Move::pchar[pt]);

        // piece letter if not pawn
        if(pt != static_cast<int>(PieceType::Pawn))
            s += p;
        
        // disambiguation
        std::vector<Move> moves = board->generate_moves();
        bool need_file = false, need_rank = false;
        int count = 0;
        for(const auto& m : moves)
        {
            if((m.piece & 0b111) == pt && m.to == move.to && m.from != move.from)
            {
                ++count;
                
                if((m.from & 0b111) != (move.from & 0b111))
                    need_file = true;
                if((m.from >> 3) != (move.from >> 3))
                    need_rank = true;
            }
        }
        if(count > 0)
        {
            if(need_file)
                s += static_cast<char>('a' + (move.from & 0b111));
            if(need_rank)
                s += static_cast<char>('1' + (move.from >> 3));
            
            if(!need_file && !need_rank)
                s += static_cast<char>('a' + (move.from & 0b111));
        }

        // capture
        if(move.flags & static_cast<uint8_t>(MoveFlag::Capture))
        {
            if(pt == static_cast<int>(PieceType::Pawn))
                s += static_cast<char>('a' + (move.from & 0b111));
            
            s += 'x';
        }

        // destination
        s += square(move.to);

        // promotion
        if(move.flags & static_cast<uint8_t>(MoveFlag::Promotion) && move.promo != 0xFF)
        {
            s += '=';
            s += std::toupper(Move::pchar[move.promo]);
        }
    }

    // check or mate
    board->make_move(move); // also makes the move
    if(board->in_check())
        s += board->generate_moves().empty() ? '#' : '+';

    return s;
}

std::string PGNExporter::date()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y.%m.%d", &tm);

    return buffer;
}

}