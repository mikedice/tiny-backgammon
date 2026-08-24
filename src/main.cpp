#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <esp_random.h>
#include <algorithm>
#include <cmath>

#include "pins.h"
#include "Game.h"
#include "BoardRenderer.h"
#include "InputController.h"
#include "ComputerPlayer.h"

Adafruit_ILI9341 display(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);

// Off-screen framebuffer: BoardRenderer draws into RAM here, then the whole
// frame is pushed to the panel in one SPI transfer. Drawing shapes directly
// to the panel (no buffering) is visibly slow enough to watch happen —
// flicker and a "jumpy" cursor — this makes each redraw appear atomic.
GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);

// Constructor pins swapped (DT, CLK) vs. physical CLK/DT wiring, so that
// turning the knob clockwise increases the encoder position.
InputController input(PIN_ENCODER_DT, PIN_ENCODER_CLK, PIN_ENCODER_SW);

int hwRollDie() {
    return static_cast<int>(esp_random() % 6) + 1;
}

Game game(hwRollDie);
UIState ui;

constexpr Player HUMAN = Player::P0;
constexpr Player COMPUTER = Player::P1;
constexpr unsigned long AI_ROLL_DELAY_MS = 700;
constexpr unsigned long AI_MOVE_DELAY_MS = 900;

constexpr unsigned long SCREENSAVER_TIMEOUT_MS = 60000;

bool screensaverActive = false;
unsigned long lastActivityAt = 0;

int wrapIndex(int idx, int size) {
    if (size <= 0) return 0;
    idx %= size;
    if (idx < 0) idx += size;
    return idx;
}

std::vector<int> distinctFroms(const std::vector<Move>& moves) {
    std::vector<int> result;
    for (const auto& m : moves) {
        if (std::find(result.begin(), result.end(), m.from) == result.end()) {
            result.push_back(m.from);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<Move> destOptionsFor(const std::vector<Move>& moves, int from) {
    std::vector<Move> result;
    for (const auto& m : moves) {
        if (m.from == from) result.push_back(m);
    }
    std::sort(result.begin(), result.end(), [](const Move& a, const Move& b) { return a.to < b.to; });
    return result;
}

void enterSelectingSource(const std::vector<Move>& legal) {
    auto froms = distinctFroms(legal);
    ui.mode = UIMode::SelectingSource;
    ui.selectedSource = -1;
    ui.cursorIndex = 0;
    ui.cursorPoint = froms.empty() ? -1 : froms[0];
}

// Draws the frame into the off-screen canvas, then pushes it to the panel
// as one SPI transfer.
void renderFrame() {
    BoardRenderer::draw(canvas, game.board(), game.toMove(), game.diceRemaining(), ui);
    display.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
}

// Purely a display state — game state underneath is untouched, so waking
// up just re-renders whatever was already there. A simple curled-up
// sleeping cat built from basic shapes, with a slowly bobbing "Z Z z" and
// a fixed high-contrast hint band pinned to the bottom so it stays legible.
constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
const uint16_t CAT_BODY = rgb565(235, 138, 55);    // orange tabby
const uint16_t CAT_STRIPE = rgb565(196, 100, 35);  // faint darker orange
const uint16_t CAT_OUTLINE = rgb565(110, 58, 20);
const uint16_t CAT_NOSE = rgb565(230, 140, 150);
const uint16_t CAT_Z = rgb565(235, 230, 255);      // soft lavender-white
const uint16_t SAVER_HINT_BG = rgb565(24, 24, 32);
constexpr int SAVER_HINT_H = 28;
constexpr unsigned long SAVER_FRAME_MS = 50; // ~20fps for a smoother drift

unsigned long screensaverStartedAt = 0;

void resetScreensaverAnimation() {
    screensaverStartedAt = millis();
}

void drawScreensaverFrame() {
    canvas.fillScreen(0x0000); // black

    // Slow wandering drift (two out-of-sync sine waves = a lazy loop, not a
    // mechanical bounce) so the cat isn't fully still.
    unsigned long t = millis() - screensaverStartedAt;
    int ox = static_cast<int>(22.0f * sinf(t / 1400.0f));
    int oy = static_cast<int>(14.0f * sinf(t / 1000.0f + 1.0f));

    // Curled body + head (head circle overlaps the body's left end).
    canvas.fillRoundRect(120 + ox, 100 + oy, 130, 55, 27, CAT_BODY);
    canvas.drawRoundRect(120 + ox, 100 + oy, 130, 55, 27, CAT_OUTLINE);
    canvas.fillCircle(130 + ox, 100 + oy, 28, CAT_BODY);
    canvas.drawCircle(130 + ox, 100 + oy, 28, CAT_OUTLINE);

    // Faint tabby stripes across the body.
    for (int sx = 150; sx <= 230; sx += 20) {
        canvas.drawLine(sx + ox, 104 + oy, sx - 6 + ox, 152 + oy, CAT_STRIPE);
        canvas.drawLine(sx + 1 + ox, 104 + oy, sx - 5 + ox, 152 + oy, CAT_STRIPE);
    }

    // Curled tail: overlapping dots tracing a sweep from the body's back,
    // with a couple of faint stripe bands.
    const int tailX[] = {245, 261, 271, 268, 254};
    const int tailY[] = {137, 128, 111, 90, 76};
    for (int i = 0; i < 5; ++i) {
        canvas.fillCircle(tailX[i] + ox, tailY[i] + oy, 7, CAT_BODY);
        canvas.drawCircle(tailX[i] + ox, tailY[i] + oy, 7, CAT_OUTLINE);
    }
    canvas.drawCircle(tailX[1] + ox, tailY[1] + oy, 4, CAT_STRIPE);
    canvas.drawCircle(tailX[3] + ox, tailY[3] + oy, 4, CAT_STRIPE);

    // Ears.
    canvas.fillTriangle(110 + ox, 83 + oy, 126 + ox, 83 + oy, 115 + ox, 57 + oy, CAT_BODY);
    canvas.drawTriangle(110 + ox, 83 + oy, 126 + ox, 83 + oy, 115 + ox, 57 + oy, CAT_OUTLINE);
    canvas.fillTriangle(134 + ox, 83 + oy, 150 + ox, 83 + oy, 145 + ox, 57 + oy, CAT_BODY);
    canvas.drawTriangle(134 + ox, 83 + oy, 150 + ox, 83 + oy, 145 + ox, 57 + oy, CAT_OUTLINE);

    // Faint forehead stripes.
    canvas.drawLine(122 + ox, 78 + oy, 130 + ox, 68 + oy, CAT_STRIPE);
    canvas.drawLine(138 + ox, 78 + oy, 130 + ox, 68 + oy, CAT_STRIPE);

    // Closed, sleepy eyes (shallow chevrons).
    canvas.drawLine(118 + ox, 99 + oy, 123 + ox, 95 + oy, CAT_OUTLINE);
    canvas.drawLine(123 + ox, 95 + oy, 128 + ox, 99 + oy, CAT_OUTLINE);
    canvas.drawLine(132 + ox, 99 + oy, 137 + ox, 95 + oy, CAT_OUTLINE);
    canvas.drawLine(137 + ox, 95 + oy, 142 + ox, 99 + oy, CAT_OUTLINE);

    // Nose and whiskers.
    canvas.fillTriangle(126 + ox, 105 + oy, 134 + ox, 105 + oy, 130 + ox, 110 + oy, CAT_NOSE);
    canvas.drawLine(120 + ox, 107 + oy, 105 + ox, 103 + oy, CAT_OUTLINE);
    canvas.drawLine(120 + ox, 111 + oy, 105 + ox, 111 + oy, CAT_OUTLINE);
    canvas.drawLine(120 + ox, 115 + oy, 105 + ox, 119 + oy, CAT_OUTLINE);

    // Gently bobbing "Z Z z", following the cat's drift.
    float phase = t / 400.0f;
    int bob = static_cast<int>(3.0f * sinf(phase));
    canvas.setTextColor(CAT_Z);
    canvas.setTextSize(3);
    canvas.setCursor(160 + ox, 44 + oy + bob);
    canvas.print("Z");
    canvas.setTextSize(2);
    canvas.setCursor(188 + ox, 29 + oy - bob);
    canvas.print("Z");
    canvas.setTextSize(1);
    canvas.setCursor(210 + ox, 18 + oy + bob);
    canvas.print("z");

    // Caption drifts left/right within its band on its own slow, independent
    // cycle — classic screensaver "keep it moving" habit, harmless on an LCD
    // but nice to keep regardless.
    const char* hintText = "asleep - click to wake"; // 22 chars
    constexpr int hintTextW = 22 * 12;                // textSize 2 => 12px/char
    constexpr int hintMargin = SCREEN_WIDTH - hintTextW;
    constexpr int hintBaseX = hintMargin / 2;
    constexpr int hintAmpX = hintBaseX - 6; // a few px of guaranteed edge margin
    int hintX = hintBaseX + static_cast<int>(hintAmpX * sinf(t / 2600.0f));
    int hintY = SCREEN_HEIGHT - SAVER_HINT_H + 6 + static_cast<int>(3.0f * sinf(t / 1900.0f + 2.0f));

    canvas.fillRect(0, SCREEN_HEIGHT - SAVER_HINT_H, SCREEN_WIDTH, SAVER_HINT_H, SAVER_HINT_BG);
    canvas.setTextColor(0xFFFF);
    canvas.setTextSize(2);
    canvas.setCursor(hintX, hintY);
    canvas.print(hintText);

    display.drawRGBBitmap(0, 0, canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
}

unsigned long lastScreensaverFrameAt = 0;

// Advances and redraws the animation, throttled to ~16fps.
void updateScreensaverAnimation() {
    unsigned long now = millis();
    if (now - lastScreensaverFrameAt < SAVER_FRAME_MS) return;
    lastScreensaverFrameAt = now;
    drawScreensaverFrame();
}

// Plays out the computer's entire turn (roll + every move) synchronously,
// with a pause before rolling and between each move so the human can watch
// it happen. No input is needed during this, so blocking delay() is fine.
void playComputerTurn() {
    ui.mode = UIMode::ComputerTurn;
    ui.selectedSource = -1;
    ui.cursorPoint = -1;

    delay(AI_ROLL_DELAY_MS);
    game.roll();
    renderFrame();

    while (game.phase() == GamePhase::MovingCheckers && game.toMove() == COMPUTER) {
        delay(AI_MOVE_DELAY_MS);
        auto legal = game.currentLegalMoves();
        if (legal.empty()) break; // Game guarantees this won't happen in this phase
        Move chosen = ComputerPlayer::chooseMove(game.board(), COMPUTER, legal);
        game.playMove(chosen);
        renderFrame();
    }

    ui.mode = (game.phase() == GamePhase::GameOver) ? UIMode::GameOver : UIMode::WaitingToRoll;
    renderFrame();
}

void setup() {
    Serial.begin(115200);

    display.begin();
    display.setRotation(1); // landscape: 320 wide x 240 tall

    if (!canvas.getBuffer()) {
        Serial.println("Canvas allocation failed - out of RAM");
        while (true) delay(1000);
    }
    Serial.printf("Free heap after canvas alloc: %u bytes\n", ESP.getFreeHeap());

    input.begin();

    renderFrame();
    lastActivityAt = millis();
}

void loop() {
    if (game.toMove() == COMPUTER && game.phase() != GamePhase::GameOver) {
        playComputerTurn();
        lastActivityAt = millis(); // don't fall asleep the instant control returns to the human
        return;
    }

    input.poll();
    int rot = input.consumeRotation();
    bool click = input.consumeClick();
    bool longPress = input.consumeLongPress();

    if (screensaverActive) {
        // Only a click wakes it, per spec — rotation/long-press are ignored
        // (and discarded) so nothing "jumps" once the game screen is back.
        if (click) {
            screensaverActive = false;
            lastActivityAt = millis();
            renderFrame();
        } else {
            updateScreensaverAnimation();
        }
        return;
    }

    if (rot != 0 || click || longPress) {
        lastActivityAt = millis();
    } else if (millis() - lastActivityAt >= SCREENSAVER_TIMEOUT_MS) {
        screensaverActive = true;
        resetScreensaverAnimation();
        drawScreensaverFrame();
        return;
    }

    bool needsRedraw = false;

    switch (game.phase()) {
        case GamePhase::WaitingToRoll: {
            ui.mode = UIMode::WaitingToRoll;
            if (longPress) {
                game.reset();
                needsRedraw = true;
            } else if (click) {
                game.roll();
                if (game.phase() == GamePhase::MovingCheckers) {
                    enterSelectingSource(game.currentLegalMoves());
                }
                needsRedraw = true;
            }
            break;
        }

        case GamePhase::MovingCheckers: {
            auto legal = game.currentLegalMoves();

            if (ui.mode == UIMode::SelectingSource) {
                auto froms = distinctFroms(legal);
                if (!froms.empty()) {
                    if (rot != 0) {
                        ui.cursorIndex = wrapIndex(ui.cursorIndex + rot, static_cast<int>(froms.size()));
                        ui.cursorPoint = froms[ui.cursorIndex];
                        needsRedraw = true;
                    }
                    if (click) {
                        ui.selectedSource = ui.cursorPoint;
                        auto dests = destOptionsFor(legal, ui.selectedSource);
                        ui.mode = UIMode::SelectingDestination;
                        ui.cursorIndex = 0;
                        ui.cursorPoint = dests.empty() ? -1 : dests[0].to;
                        ui.currentDie = dests.empty() ? 0 : dests[0].die;
                        needsRedraw = true;
                    }
                }
            } else if (ui.mode == UIMode::SelectingDestination) {
                auto dests = destOptionsFor(legal, ui.selectedSource);
                if (!dests.empty()) {
                    if (rot != 0) {
                        ui.cursorIndex = wrapIndex(ui.cursorIndex + rot, static_cast<int>(dests.size()));
                        ui.cursorPoint = dests[ui.cursorIndex].to;
                        ui.currentDie = dests[ui.cursorIndex].die;
                        needsRedraw = true;
                    }
                    if (longPress) {
                        enterSelectingSource(legal);
                        needsRedraw = true;
                    } else if (click) {
                        Move mv{ui.selectedSource, ui.cursorPoint, ui.currentDie};
                        game.playMove(mv);

                        if (game.phase() == GamePhase::MovingCheckers) {
                            enterSelectingSource(game.currentLegalMoves());
                        } else if (game.phase() == GamePhase::GameOver) {
                            ui.mode = UIMode::GameOver;
                        } else {
                            ui.mode = UIMode::WaitingToRoll;
                        }
                        needsRedraw = true;
                    }
                }
            }
            break;
        }

        case GamePhase::GameOver: {
            ui.mode = UIMode::GameOver;
            if (click) {
                game.reset();
                ui.mode = UIMode::WaitingToRoll;
                needsRedraw = true;
            }
            break;
        }
    }

    if (needsRedraw) {
        renderFrame();
    }
}
