#include "nebula/TranspositionTable.hpp"

namespace nebula
{

TranspositionTable::TranspositionTable():
    table(kNumEntries) {}

void TranspositionTable::clear()
{
    for(auto& entry : table)
        entry = TTEntry{};
}

const TTEntry* TranspositionTable::probe(uint64_t key) const
{
    const TTEntry& entry = table[key & kIndexMask];

    return entry.is_valid(key) ? &entry : nullptr;
}

TTEntry* TranspositionTable::probe(uint64_t key)
{
    TTEntry& entry = table[key & kIndexMask];

    return entry.is_valid(key) ? &entry : nullptr;
}

void TranspositionTable::store(uint64_t key, int eval, int depth, TTFlag flag, Move move)
{
    TTEntry& entry = table[key & kIndexMask];

    if (!entry.is_valid(key) || depth >= entry.depth)
    {
        entry.key = key;
        entry.eval = eval;
        entry.depth = depth;
        entry.flag = flag;
        entry.move = move;
    }
}

}