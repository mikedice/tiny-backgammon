#pragma once
#include "Types.h"
#include <array>

class Board {
public:
    Board() = default;

    // points[i] holds absolute point (i+1). Positive = P0 checkers,
    // negative = P1 checkers, 0 = empty. Starts empty; call
    // resetStartingPosition() for the standard opening layout.
    std::array<int8_t, NUM_POINTS> points{};
    uint8_t bar[2] = {0, 0}; // indexed by static_cast<int>(Player)
    uint8_t off[2] = {0, 0};

    void resetStartingPosition();

    int checkersOf(Player p, int point) const;
    bool isBlocked(Player p, int point) const;
    bool allCheckersHome(Player p) const;

    int direction(Player p) const { return p == Player::P0 ? -1 : 1; }
    int homeStart(Player p) const { return p == Player::P0 ? 1 : 19; }
    int homeEnd(Player p) const { return p == Player::P0 ? 6 : 24; }

    // Applies a move that has already been validated (see MoveGen). Handles
    // hitting a blot (sends it to the bar) and bearing off.
    void applyMove(Player p, const Move& m);

    bool isGameOver() const;
    Player winner() const; // valid only when isGameOver()

private:
    void addChecker(Player p, int point, int8_t delta);
};
