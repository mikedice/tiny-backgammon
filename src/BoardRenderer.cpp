#include "BoardRenderer.h"

// Layout (128x64):
//   y 0..7    status line
//   y 9..32   top-row triangles (13-18 | bar | 19-24), base at 9, apex at 33
//   y 33..39  middle gap — cursor carets live here
//   y 39..63  bottom-row triangles (12-7 | bar | 6-1), apex at 39, base at 63
//   x 0..59   left half (6 slots x 10px)
//   x 60..67  bar column
//   x 68..127 right half (6 slots x 10px)
namespace {

constexpr int BOARD_TOP = 9;
constexpr int BOARD_BOTTOM = 63;
constexpr int TRIANGLE_H = 24;
constexpr int SLOT_W = 10;
constexpr int BAR_X = 60;
constexpr int BAR_W = 8;
constexpr int RIGHT_HALF_X = BAR_X + BAR_W;
constexpr int CHECKER_R = 3;

struct PointGeom {
    int x;    // left edge of the slot
    bool top; // true = top row (triangle points down from the top edge)
};

PointGeom pointGeom(int point) {
    if (point >= 13 && point <= 18) return {(point - 13) * SLOT_W, true};
    if (point >= 19 && point <= 24) return {RIGHT_HALF_X + (point - 19) * SLOT_W, true};
    if (point >= 7 && point <= 12) return {(12 - point) * SLOT_W, false};
    return {RIGHT_HALF_X + (6 - point) * SLOT_W, false}; // 1..6
}

void drawBoard(Adafruit_SSD1306& d) {
    for (int point = 1; point <= 24; ++point) {
        PointGeom g = pointGeom(point);
        int xMid = g.x + SLOT_W / 2;
        if (g.top) {
            d.drawTriangle(g.x, BOARD_TOP, g.x + SLOT_W, BOARD_TOP, xMid, BOARD_TOP + TRIANGLE_H, SSD1306_WHITE);
        } else {
            d.drawTriangle(g.x, BOARD_BOTTOM, g.x + SLOT_W, BOARD_BOTTOM, xMid, BOARD_BOTTOM - TRIANGLE_H, SSD1306_WHITE);
        }
    }
    d.drawRect(BAR_X, BOARD_TOP, BAR_W, BOARD_BOTTOM - BOARD_TOP, SSD1306_WHITE);
}

// Stacks up to 3 individual checkers from the triangle's base inward; beyond
// that, shows 2 checkers plus a numeral for the true count.
void drawCheckerStack(Adafruit_SSD1306& d, int xMid, int baseY, int count, bool growDown, bool filled) {
    if (count <= 0) return;
    int shown = count <= 3 ? count : 2;
    int step = CHECKER_R * 2 + 1;

    for (int i = 0; i < shown; ++i) {
        int cy = growDown ? (baseY + CHECKER_R + i * step) : (baseY - CHECKER_R - i * step);
        if (filled) d.fillCircle(xMid, cy, CHECKER_R, SSD1306_WHITE);
        else d.drawCircle(xMid, cy, CHECKER_R, SSD1306_WHITE);
    }

    if (count > 3) {
        int cy = growDown ? (baseY + CHECKER_R + shown * step) : (baseY - CHECKER_R - shown * step);
        d.setTextSize(1);
        d.setTextColor(SSD1306_WHITE);
        d.setCursor(xMid - 3, cy - 3);
        d.print(count);
    }
}

void drawCaret(Adafruit_SSD1306& d, int point, bool hollow) {
    PointGeom g = pointGeom(point);
    int xMid = g.x + SLOT_W / 2;
    // apex points into the middle gap toward the triangle it indicates.
    if (g.top) {
        if (hollow) d.drawTriangle(xMid - 3, 37, xMid + 3, 37, xMid, 33, SSD1306_WHITE);
        else d.fillTriangle(xMid - 3, 37, xMid + 3, 37, xMid, 33, SSD1306_WHITE);
    } else {
        if (hollow) d.drawTriangle(xMid - 3, 35, xMid + 3, 35, xMid, 39, SSD1306_WHITE);
        else d.fillTriangle(xMid - 3, 35, xMid + 3, 35, xMid, 39, SSD1306_WHITE);
    }
}

} // namespace

void BoardRenderer::draw(Adafruit_SSD1306& display, const Board& board, Player toMove,
                          const std::vector<int>& diceRemaining, const UIState& ui) {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false); // never let an overlong status line spill onto the board
    display.setCursor(0, 0);
    display.print(toMove == Player::P0 ? "P0 " : "P1 ");
    switch (ui.mode) {
        case UIMode::WaitingToRoll: display.print("ROLL"); break;
        case UIMode::SelectingSource: display.print("PICK"); break;
        case UIMode::SelectingDestination: display.print("DEST"); break;
        case UIMode::GameOver: display.print("WINS!"); break;
    }
    if (!diceRemaining.empty()) {
        // Concatenated digits (e.g. "4444" for doubles) — every die value is
        // a single digit, so no separators are needed and it stays compact
        // enough to never wrap off the 128px status line.
        display.print(' ');
        for (int d : diceRemaining) display.print(d);
    }
    if ((ui.mode == UIMode::SelectingSource || ui.mode == UIMode::SelectingDestination)) {
        if (ui.cursorPoint == BAR) display.print(" BAR");
        else if (ui.cursorPoint == OFF) display.print(" OFF");
    }

    drawBoard(display);

    for (int point = 1; point <= 24; ++point) {
        PointGeom g = pointGeom(point);
        int xMid = g.x + SLOT_W / 2;
        int baseY = g.top ? BOARD_TOP : BOARD_BOTTOM;
        int p0 = board.checkersOf(Player::P0, point);
        int p1 = board.checkersOf(Player::P1, point);
        if (p0 > 0) drawCheckerStack(display, xMid, baseY, p0, g.top, true);
        if (p1 > 0) drawCheckerStack(display, xMid, baseY, p1, g.top, false);
    }

    int barXMid = BAR_X + BAR_W / 2;
    drawCheckerStack(display, barXMid, BOARD_TOP, board.bar[static_cast<int>(Player::P0)], true, true);
    drawCheckerStack(display, barXMid, BOARD_BOTTOM, board.bar[static_cast<int>(Player::P1)], false, false);

    if (ui.mode == UIMode::SelectingDestination && ui.selectedSource >= 1 && ui.selectedSource <= 24) {
        drawCaret(display, ui.selectedSource, true);
    }
    if ((ui.mode == UIMode::SelectingSource || ui.mode == UIMode::SelectingDestination) &&
        ui.cursorPoint >= 1 && ui.cursorPoint <= 24) {
        drawCaret(display, ui.cursorPoint, false);
    }

    display.display();
}
