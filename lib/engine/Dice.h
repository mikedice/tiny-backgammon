#pragma once
#include <functional>
#include <vector>

// Injectable die-roll source: returns a value in 1..6. Native builds/tests
// can supply a scripted deterministic source; hardware builds supply a real
// RNG (e.g. esp_random()).
using DieRoller = std::function<int()>;

struct Roll {
    int a;
    int b;

    bool isDouble() const { return a == b; }

    // Dice values to play this turn: 2 entries normally, 4 if doubles.
    std::vector<int> toDice() const {
        if (isDouble()) return {a, a, a, a};
        return {a, b};
    }
};

Roll rollDice(const DieRoller& roller);
