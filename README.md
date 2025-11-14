# Nebula Chess Engine

Nebula is a C++ chess engine built around a fast bitboard core, a modern alpha–beta search, and a small Python toolkit for tuning the static evaluation.

## Overview

Nebula has three main components: how positions are stored, how moves are searched, and how positions are evaluated.

### 1. Board Representation
Pieces are stored as 64-bit bitboards, which makes it straightforward to:
- test occupancy,
- generate moves with bit operations,
- make and unmake moves efficiently.

Knight, king, and pawn attacks are precomputed. Sliding-piece attacks (rooks, bishops, queens) use magic bitboards, allowing near-instant lookup of legal moves based on the current position.

### 2. Search
The search is a negamax alpha–beta framework with layers of practical improvements:
- **Iterative deepening**, which lets the engine refine move choices as depth increases
- **Transposition table** to reuse earlier results
- **Move ordering** driven by:
  - best move from the previous iteration,
  - killer moves,
  - a history heuristic,
  - MVV-LVA captures
- **Pruning and reductions** including:
  - null-move pruning,
  - futility and reverse futility pruning,
  - late-move pruning and reductions
- **Quiescence search** to handle tactical volatility at the leaves

### 3. Evaluation
The evaluation blends opening and endgame values based on game phase. It scores:
- **Material** and piece placement (via piece-square tables)
- **King safety**, including castling rights and castled positions
- **Pawn structure**:
  - isolated, doubled, and backward pawn weaknesses
  - passed pawns, with bonuses for being advanced, connected, or protected

## Features

### Engine
- Bitboard move generation with precomputed attack tables
- Magic bitboards for sliding pieces
- Alpha–beta search with iterative deepening
- Strong move ordering and pruning heuristics
- Quiescence search
- Zobrist hashing and transposition table
- Detection of repetition, stalemate, and 50-move rule

### Gameplay Modes
- **PVE**: user enters UCI moves
- **EVE**: engine plays both sides and prints moves with evaluations
- Automatic PGN exporting with SAN notation

### Python Tuning
- Loads positions and results from CSV
- Trains evaluation weights using cross-entropy loss
- Finite-difference gradients
- Saves tuned parameters to JSON

### PGN Export
- SAN conversion and tagging (date, result, etc.)
- Outputs complete PGNs at the end of games

### CLI Interface
- `--mode` (required): PVE or EVE
- `--depth`: search depth
- `--length`: max game length
- `--help`: usage guide

## Project Layout

```
Nebula-Chess-Engine/
├── include/
│   └── nebula/
│       ├── AttackTables.hpp
│       ├── Board.hpp
│       ├── CLIHelper.hpp
│       ├── Driver.hpp
│       ├── Evaluate.hpp
│       ├── MagicBitboards.hpp
│       ├── PGNExporter.hpp
│       ├── Search.hpp
│       ├── TranspositionTable.hpp
│       └── Values.hpp
│
├── src/
│   ├── main.cpp
│   ├── AttackTables.cpp
│   ├── Board.cpp
│   ├── CLIHelper.cpp
│   ├── Driver.cpp
│   ├── Evaluate.cpp
│   ├── MagicBitboards.cpp
│   ├── PGNExporter.cpp
│   ├── Search.cpp
│   └── TranspositionTable.cpp
│
└── tuning/
    ├── data/
    │   ├── training_positions.csv
    │   └── validation_positions.csv
    └── scripts/
        ├── chess_evaluation/
        │   ├── __init__.py
        │   ├── evaluate.py
        │   └── types.py
        └── tune_evaluation.py
```

## Building

The Makefile compiles everything in one step using clang++ with aggressive optimization and LTO.

```bash
make
```

This produces the `nebula` binary in the project root.

To clean:

```bash
make clean
```

## Running

Nebula has two modes:  
- **PVE** — you play against the engine
- **EVE** — the engine plays itself and prints moves and evaluations

The mode is required. Depth and game length are optional.

### Basic usage

```bash
./nebula -m MODE [OPTIONS]
```

### Required

```
-m, --mode MODE
    PVE    Player vs Engine (you enter moves in UCI)
    EVE    Engine vs Engine (auto-play)
```

### Optional

```
-d, --depth DEPTH
    Search depth (default: 8)

-l, --length LENGTH
    Maximum number of moves (default: unlimited)

-h, --help
    Show help message
```

### Examples

Play against the engine at depth 6:
```bash
./nebula -m PVE --depth 6
```

Run engine-vs-engine at depth 10 for up to 200 moves:
```bash
./nebula --mode EVE -d 10 -l 200
```

### Player vs Engine (PVE)

Nebula prints the board, then waits for you to enter moves in UCI format:
```
Enter move in UCI format:
e2e4
```

After the engine replies, the board is shown again, along with its move:
```
Engine played e7e5
```

Illegal moves prompt a retry.
Nebula handles:
- checkmate
- stalemate
- 50-move rule draws
- repetition draws

All results are stored using the PGN exporter and printed at the end.

### Engine vs Engine (EVE)

Nebula prints each engine move along with the evaluation:
```
0.40: e2e4
0.20: e7e6
```

After every move, the updated ASCII board is shown.

It stops on:
- checkmate
- stalemate
- 50-move rule draws
- repetition draws

A complete PGN is printed when the game ends.