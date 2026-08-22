#pragma once
#include <Adafruit_SSD1306.h>
#include <vector>
#include "Board.h"
#include "Types.h"

enum class UIMode { WaitingToRoll, SelectingSource, SelectingDestination, GameOver };

struct UIState {
    UIMode mode = UIMode::WaitingToRoll;
    int cursorPoint = 0;    // BAR(0), 1..24, or OFF(25) — point currently focused
    int cursorIndex = 0;    // index into whatever candidate list is active
    int selectedSource = -1; // locked-in source point, once chosen (-1 = none)
    int currentDie = 0;     // die value the current destination candidate uses
};

class BoardRenderer {
public:
    static void draw(Adafruit_SSD1306& display, const Board& board, Player toMove,
                      const std::vector<int>& diceRemaining, const UIState& ui);
};
