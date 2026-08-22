#include "Board.h"

void Board::resetStartingPosition() {
    points.fill(0);
    bar[0] = bar[1] = 0;
    off[0] = off[1] = 0;

    // Standard mirrored opening position.
    points[23] = 2;   // P0: point 24
    points[12] = 5;   // P0: point 13
    points[7] = 3;    // P0: point 8
    points[5] = 5;    // P0: point 6

    points[0] = -2;   // P1: point 1
    points[11] = -5;  // P1: point 12
    points[16] = -3;  // P1: point 17
    points[18] = -5;  // P1: point 19
}

int Board::checkersOf(Player p, int point) const {
    int8_t v = points[point - 1];
    if (p == Player::P0) return v > 0 ? v : 0;
    return v < 0 ? -v : 0;
}

bool Board::isBlocked(Player p, int point) const {
    return checkersOf(opponent(p), point) >= 2;
}

bool Board::allCheckersHome(Player p) const {
    if (bar[static_cast<int>(p)] > 0) return false;
    int start = homeStart(p);
    int end = homeEnd(p);
    for (int pt = 1; pt <= NUM_POINTS; ++pt) {
        if (pt >= start && pt <= end) continue;
        if (checkersOf(p, pt) > 0) return false;
    }
    return true;
}

void Board::addChecker(Player p, int point, int8_t delta) {
    int8_t sign = (p == Player::P0) ? 1 : -1;
    points[point - 1] += static_cast<int8_t>(sign * delta);
}

void Board::applyMove(Player p, const Move& m) {
    if (m.from == BAR) {
        bar[static_cast<int>(p)]--;
    } else {
        addChecker(p, m.from, -1);
    }

    if (m.to == OFF) {
        off[static_cast<int>(p)]++;
        return;
    }

    Player opp = opponent(p);
    if (checkersOf(opp, m.to) == 1) {
        addChecker(opp, m.to, -1);
        bar[static_cast<int>(opp)]++;
    }

    addChecker(p, m.to, 1);
}

bool Board::isGameOver() const {
    return off[0] == 15 || off[1] == 15;
}

Player Board::winner() const {
    return off[0] == 15 ? Player::P0 : Player::P1;
}
