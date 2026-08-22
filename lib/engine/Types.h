#pragma once
#include <cstdint>

enum class Player : uint8_t { P0 = 0, P1 = 1 };

inline Player opponent(Player p) {
    return p == Player::P0 ? Player::P1 : Player::P0;
}

// Point numbering: 1..24 absolute board points, matching physical layout.
// P0 moves 24 -> 1 and bears off past point 1.
// P1 moves 1 -> 24 and bears off past point 24.
static constexpr int BAR = 0;
static constexpr int OFF = 25;
static constexpr int NUM_POINTS = 24;

struct Move {
    int from; // BAR (0) or 1..24
    int to;   // 1..24 or OFF (25)
    int die;  // pip value 1..6 used for this move
};
