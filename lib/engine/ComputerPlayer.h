#pragma once
#include "Board.h"
#include <vector>

// A simple greedy heuristic move-picker for an AI opponent: prefers hitting
// blots, making/reinforcing points, bearing off, and avoiding new blots of
// its own. No lookahead beyond the single move being scored.
namespace ComputerPlayer {
    // legalMoves must be non-empty.
    Move chooseMove(const Board& board, Player p, const std::vector<Move>& legalMoves);
}
