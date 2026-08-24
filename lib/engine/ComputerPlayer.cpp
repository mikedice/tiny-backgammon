#include "ComputerPlayer.h"

namespace ComputerPlayer {
namespace {

int scoreMove(const Board& board, Player p, const Move& m) {
    int score = 0;
    Player opp = opponent(p);

    if (m.to != OFF && board.checkersOf(opp, m.to) == 1) {
        score += 50; // hits a blot
    }

    if (m.to == OFF) {
        score += 15; // bearing off
    } else {
        int destAfter = board.checkersOf(p, m.to) + 1;
        if (destAfter >= 2) score += 20; // makes/reinforces a point
        else score -= 15;                 // leaves a blot there
    }

    if (m.from != BAR && board.checkersOf(p, m.from) == 2) {
        score -= 10; // breaks up an existing point
    }

    score += m.die; // small tiebreak toward using the bigger die

    return score;
}

} // namespace

Move chooseMove(const Board& board, Player p, const std::vector<Move>& legalMoves) {
    const Move* best = &legalMoves[0];
    int bestScore = scoreMove(board, p, legalMoves[0]);
    for (size_t i = 1; i < legalMoves.size(); ++i) {
        int s = scoreMove(board, p, legalMoves[i]);
        if (s > bestScore) {
            bestScore = s;
            best = &legalMoves[i];
        }
    }
    return *best;
}

} // namespace ComputerPlayer
