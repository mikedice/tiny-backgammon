#include "MoveGen.h"
#include <algorithm>

bool MoveGen::isPseudoLegal(const Board& board, Player p, const Move& m) {
    int pi = static_cast<int>(p);
    bool hasBarCheckers = board.bar[pi] > 0;

    if (hasBarCheckers && m.from != BAR) return false;
    if (!hasBarCheckers && m.from == BAR) return false;

    int dir = board.direction(p);

    if (m.from == BAR) {
        int entry = (p == Player::P0) ? (25 - m.die) : m.die;
        if (m.to != entry) return false;
        return !board.isBlocked(p, entry);
    }

    if (m.from < 1 || m.from > NUM_POINTS) return false;
    if (board.checkersOf(p, m.from) <= 0) return false;

    int rawTo = m.from + dir * m.die;
    bool overshoots = (p == Player::P0) ? (rawTo < 1) : (rawTo > NUM_POINTS);
    bool exactOff = (p == Player::P0) ? (rawTo == 0) : (rawTo == NUM_POINTS + 1);

    if (overshoots || exactOff) {
        if (m.to != OFF) return false;
        if (!board.allCheckersHome(p)) return false;
        if (exactOff) return true;

        // Overshoot bear-off only legal if no checker sits further from home
        // (i.e. behind `from`) within the home board.
        if (p == Player::P0) {
            for (int pt = m.from + 1; pt <= board.homeEnd(p); ++pt) {
                if (board.checkersOf(p, pt) > 0) return false;
            }
        } else {
            for (int pt = board.homeStart(p); pt < m.from; ++pt) {
                if (board.checkersOf(p, pt) > 0) return false;
            }
        }
        return true;
    }

    if (m.to != rawTo) return false;
    return !board.isBlocked(p, rawTo);
}

std::vector<Move> MoveGen::allPseudoLegalMoves(const Board& board, Player p,
                                                const std::vector<int>& diceRemaining) {
    std::vector<Move> result;
    bool seen[7] = {false, false, false, false, false, false, false};
    int pi = static_cast<int>(p);
    int dir = board.direction(p);

    for (int d : diceRemaining) {
        if (d < 1 || d > 6 || seen[d]) continue;
        seen[d] = true;

        if (board.bar[pi] > 0) {
            int entry = (p == Player::P0) ? (25 - d) : d;
            Move m{BAR, entry, d};
            if (isPseudoLegal(board, p, m)) result.push_back(m);
            continue;
        }

        for (int pt = 1; pt <= NUM_POINTS; ++pt) {
            if (board.checkersOf(p, pt) <= 0) continue;
            int rawTo = pt + dir * d;
            int to = (rawTo < 1 || rawTo > NUM_POINTS) ? OFF : rawTo;
            Move m{pt, to, d};
            if (isPseudoLegal(board, p, m)) result.push_back(m);
        }
    }

    return result;
}

int MoveGen::maxPlyLength(const Board& board, Player p, const std::vector<int>& diceRemaining) {
    if (diceRemaining.empty()) return 0;

    int best = 0;
    for (const auto& m : allPseudoLegalMoves(board, p, diceRemaining)) {
        Board next = board;
        next.applyMove(p, m);

        std::vector<int> rest = diceRemaining;
        rest.erase(std::find(rest.begin(), rest.end(), m.die));

        int len = 1 + maxPlyLength(next, p, rest);
        if (len > best) best = len;
    }
    return best;
}

std::vector<Move> MoveGen::legalMoves(const Board& board, Player p,
                                       const std::vector<int>& diceRemaining) {
    if (diceRemaining.empty()) return {};

    int target = maxPlyLength(board, p, diceRemaining);
    if (target == 0) return {};

    std::vector<Move> keepers;
    for (const auto& m : allPseudoLegalMoves(board, p, diceRemaining)) {
        Board next = board;
        next.applyMove(p, m);

        std::vector<int> rest = diceRemaining;
        rest.erase(std::find(rest.begin(), rest.end(), m.die));

        if (1 + maxPlyLength(next, p, rest) == target) {
            keepers.push_back(m);
        }
    }

    bool isDouble = diceRemaining.size() >= 2 &&
        std::all_of(diceRemaining.begin(), diceRemaining.end(),
                    [&](int d) { return d == diceRemaining[0]; });

    if (!isDouble && diceRemaining.size() == 2 && target == 1) {
        int d0 = diceRemaining[0];
        int d1 = diceRemaining[1];
        if (d0 != d1) {
            bool hasD0 = std::any_of(keepers.begin(), keepers.end(),
                                      [&](const Move& m) { return m.die == d0; });
            bool hasD1 = std::any_of(keepers.begin(), keepers.end(),
                                      [&](const Move& m) { return m.die == d1; });
            if (hasD0 && hasD1) {
                int larger = std::max(d0, d1);
                std::vector<Move> filtered;
                for (auto& m : keepers) {
                    if (m.die == larger) filtered.push_back(m);
                }
                return filtered;
            }
        }
    }

    return keepers;
}
