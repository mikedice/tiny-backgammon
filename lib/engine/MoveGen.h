#pragma once
#include "Board.h"
#include <vector>

class MoveGen {
public:
    // Legal single-step moves available right now, given the board, the
    // player to move, and the dice still unplayed this turn. Already
    // filtered by the official rules: a player must use as many dice as
    // possible, and if a non-double roll allows only one die to be played
    // (either one, but not both), the larger die must be the one played.
    static std::vector<Move> legalMoves(const Board& board, Player p,
                                         const std::vector<int>& diceRemaining);

    // Max number of dice (out of diceRemaining) playable from this position,
    // considering every ordering and checker choice.
    static int maxPlyLength(const Board& board, Player p,
                             const std::vector<int>& diceRemaining);

private:
    static bool isPseudoLegal(const Board& board, Player p, const Move& m);
    static std::vector<Move> allPseudoLegalMoves(const Board& board, Player p,
                                                  const std::vector<int>& diceRemaining);
};
