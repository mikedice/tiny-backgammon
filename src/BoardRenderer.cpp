#include "BoardRenderer.h"

// Layout (320x240 landscape):
//   y 0..21     status bar
//   y 22..109   top-row triangles (13-18 | bar | 19-24), base at 22, apex at 110
//   y 110..151  middle gap — cursor carets live here
//   y 151..239  bottom-row triangles (12-7 | bar | 6-1), apex at 151, base at 239
//   x 0..149    left half (6 slots x 25px)
//   x 150..169  bar column
//   x 170..319  right half (6 slots x 25px)
namespace {

constexpr int STATUS_H = 22;
constexpr int BOARD_TOP = 22;
constexpr int BOARD_BOTTOM = 239;
constexpr int TRIANGLE_H = 88;
constexpr int SLOT_W = 25;
constexpr int BAR_X = 150;
constexpr int BAR_W = 20;
constexpr int RIGHT_HALF_X = BAR_X + BAR_W;
constexpr int CHECKER_R = 8;

constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

const uint16_t COLOR_BG = rgb565(101, 67, 33);        // walnut background
const uint16_t COLOR_POINT_A = rgb565(214, 184, 140); // tan
const uint16_t COLOR_POINT_B = rgb565(139, 94, 60);   // medium brown
const uint16_t COLOR_BAR = rgb565(58, 36, 24);        // near-black walnut
const uint16_t COLOR_OUTLINE = rgb565(35, 20, 12);
const uint16_t COLOR_STATUS_BG = rgb565(24, 24, 24);
const uint16_t COLOR_TEXT = rgb565(255, 255, 255);

const uint16_t COLOR_P0_FILL = rgb565(250, 248, 240); // human: ivory
const uint16_t COLOR_P0_EDGE = rgb565(40, 40, 40);
const uint16_t COLOR_P1_FILL = rgb565(160, 24, 24);   // computer: deep red
const uint16_t COLOR_P1_EDGE = rgb565(255, 230, 190);

const uint16_t COLOR_CURSOR = rgb565(255, 200, 40); // gold

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

void drawBoard(Adafruit_GFX& d) {
    d.fillRect(0, STATUS_H, d.width(), d.height() - STATUS_H, COLOR_BG);

    for (int point = 1; point <= 24; ++point) {
        PointGeom g = pointGeom(point);
        int xMid = g.x + SLOT_W / 2;
        uint16_t color = (point % 2 == 0) ? COLOR_POINT_A : COLOR_POINT_B;
        if (g.top) {
            d.fillTriangle(g.x, BOARD_TOP, g.x + SLOT_W, BOARD_TOP, xMid, BOARD_TOP + TRIANGLE_H, color);
            d.drawTriangle(g.x, BOARD_TOP, g.x + SLOT_W, BOARD_TOP, xMid, BOARD_TOP + TRIANGLE_H, COLOR_OUTLINE);
        } else {
            d.fillTriangle(g.x, BOARD_BOTTOM, g.x + SLOT_W, BOARD_BOTTOM, xMid, BOARD_BOTTOM - TRIANGLE_H, color);
            d.drawTriangle(g.x, BOARD_BOTTOM, g.x + SLOT_W, BOARD_BOTTOM, xMid, BOARD_BOTTOM - TRIANGLE_H, COLOR_OUTLINE);
        }
    }

    d.fillRect(BAR_X, BOARD_TOP, BAR_W, BOARD_BOTTOM - BOARD_TOP, COLOR_BAR);
    d.drawRect(BAR_X, BOARD_TOP, BAR_W, BOARD_BOTTOM - BOARD_TOP, COLOR_OUTLINE);
    d.drawRect(0, BOARD_TOP, d.width(), BOARD_BOTTOM - BOARD_TOP, COLOR_OUTLINE);
}

// Stacks individual checkers from the triangle's base inward; a full 5-high
// stack fits without crowding. Beyond that, shows 4 checkers plus a numeral
// for the true count.
void drawCheckerStack(Adafruit_GFX& d, int xMid, int baseY, int count, bool growDown, Player p) {
    if (count <= 0) return;
    uint16_t fill = (p == Player::P0) ? COLOR_P0_FILL : COLOR_P1_FILL;
    uint16_t edge = (p == Player::P0) ? COLOR_P0_EDGE : COLOR_P1_EDGE;

    int shown = count <= 5 ? count : 4;
    int step = CHECKER_R * 2 + 1;

    for (int i = 0; i < shown; ++i) {
        int cy = growDown ? (baseY + CHECKER_R + i * step) : (baseY - CHECKER_R - i * step);
        d.fillCircle(xMid, cy, CHECKER_R, fill);
        d.drawCircle(xMid, cy, CHECKER_R, edge);
    }

    if (count > 5) {
        int cy = growDown ? (baseY + CHECKER_R + shown * step) : (baseY - CHECKER_R - shown * step);
        d.setTextSize(1);
        d.setTextColor(COLOR_TEXT);
        d.setCursor(xMid - 4, cy - 4);
        d.print(count);
    }
}

void drawCaret(Adafruit_GFX& d, int point, bool hollow) {
    PointGeom g = pointGeom(point);
    int xMid = g.x + SLOT_W / 2;
    // apex points into the middle gap toward the triangle it indicates.
    if (g.top) {
        if (hollow) d.drawTriangle(xMid - 8, 122, xMid + 8, 122, xMid, 110, COLOR_CURSOR);
        else d.fillTriangle(xMid - 8, 122, xMid + 8, 122, xMid, 110, COLOR_CURSOR);
    } else {
        if (hollow) d.drawTriangle(xMid - 8, 139, xMid + 8, 139, xMid, 151, COLOR_CURSOR);
        else d.fillTriangle(xMid - 8, 139, xMid + 8, 139, xMid, 151, COLOR_CURSOR);
    }
}

void drawBarCaret(Adafruit_GFX& d, bool hollow) {
    int xMid = BAR_X + BAR_W / 2;
    if (hollow) d.drawRect(xMid - 8, 122, 16, 12, COLOR_CURSOR);
    else d.fillRect(xMid - 8, 122, 16, 12, COLOR_CURSOR);
}

void drawOffCaret(Adafruit_GFX& d, Player p, bool hollow) {
    int y = (p == Player::P0) ? (BOARD_BOTTOM - 20) : (BOARD_TOP + 6);
    if (hollow) d.drawRect(298, y, 14, 14, COLOR_CURSOR);
    else d.fillRect(298, y, 14, 14, COLOR_CURSOR);
}

} // namespace

void BoardRenderer::draw(Adafruit_GFX& display, const Board& board, Player toMove,
                          const std::vector<int>& diceRemaining, const UIState& ui) {
    display.fillRect(0, 0, display.width(), STATUS_H, COLOR_STATUS_BG);

    display.setTextSize(2);
    display.setTextColor(COLOR_TEXT);
    display.setCursor(4, 3);
    display.print(toMove == Player::P0 ? "P0 " : "P1 ");
    switch (ui.mode) {
        case UIMode::WaitingToRoll: display.print("ROLL"); break;
        case UIMode::SelectingSource: display.print("PICK"); break;
        case UIMode::SelectingDestination: display.print("DEST"); break;
        case UIMode::ComputerTurn: display.print("CPU"); break;
        case UIMode::GameOver: display.print("WINS!"); break;
    }
    if (!diceRemaining.empty()) {
        display.print(' ');
        for (int d : diceRemaining) display.print(d);
    }
    if (ui.mode == UIMode::SelectingSource || ui.mode == UIMode::SelectingDestination) {
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
        if (p0 > 0) drawCheckerStack(display, xMid, baseY, p0, g.top, Player::P0);
        if (p1 > 0) drawCheckerStack(display, xMid, baseY, p1, g.top, Player::P1);
    }

    int barXMid = BAR_X + BAR_W / 2;
    drawCheckerStack(display, barXMid, BOARD_TOP, board.bar[static_cast<int>(Player::P0)], true, Player::P0);
    drawCheckerStack(display, barXMid, BOARD_BOTTOM, board.bar[static_cast<int>(Player::P1)], false, Player::P1);

    if (ui.mode == UIMode::SelectingDestination) {
        if (ui.selectedSource >= 1 && ui.selectedSource <= 24) drawCaret(display, ui.selectedSource, true);
        else if (ui.selectedSource == BAR) drawBarCaret(display, true);
    }
    if (ui.mode == UIMode::SelectingSource || ui.mode == UIMode::SelectingDestination) {
        if (ui.cursorPoint >= 1 && ui.cursorPoint <= 24) drawCaret(display, ui.cursorPoint, false);
        else if (ui.cursorPoint == BAR) drawBarCaret(display, false);
        else if (ui.cursorPoint == OFF) drawOffCaret(display, toMove, false);
    }
}
