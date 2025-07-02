#ifndef NEBULA_TRANSPOSITIONTABLE_HPP
#define NEBULA_TRANSPOSITIONTABLE_HPP

#include "nebula/Board.hpp"

namespace nebula
{

enum class TTFlag : uint8_t { Exact, LowerBound, UpperBound };

struct TTEntry
{
    uint64_t key = 0;
    int eval = 0;
    int depth = -1;
    TTFlag flag = TTFlag::Exact;
    Move move;

    inline bool is_valid(uint64_t probe_key) const { return key == probe_key; }
};

class TranspositionTable
{
public:
    TranspositionTable();

    void clear();

    // returns pointer to entry or nullptr
    const TTEntry* probe(uint64_t key) const;
    TTEntry* probe(uint64_t key);

    // store new entry
    void store(uint64_t key, int eval, int depth, TTFlag flag, Move move);

private:
    static constexpr size_t kNumEntries = 1 << 20; // 1M entries (~16 MB)
    static constexpr size_t kIndexMask = kNumEntries - 1;

    std::vector<TTEntry> table;
};

}

#endif