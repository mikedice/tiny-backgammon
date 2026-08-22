#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_random.h>
#include <algorithm>

#include "pins.h"
#include "Game.h"
#include "BoardRenderer.h"
#include "InputController.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Constructor pins swapped (DT, CLK) vs. physical CLK/DT wiring, so that
// turning the knob clockwise increases the encoder position.
InputController input(PIN_ENCODER_DT, PIN_ENCODER_CLK, PIN_ENCODER_SW);

int hwRollDie() {
    return static_cast<int>(esp_random() % 6) + 1;
}

Game game(hwRollDie);
UIState ui;

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

void setup() {
    Serial.begin(115200);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDR)) {
        Serial.println("SSD1306 allocation failed");
        while (true) delay(1000);
    }

    input.begin();

    BoardRenderer::draw(display, game.board(), game.toMove(), game.diceRemaining(), ui);
}

void loop() {
    input.poll();
    int rot = input.consumeRotation();
    bool click = input.consumeClick();
    bool longPress = input.consumeLongPress();

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
        BoardRenderer::draw(display, game.board(), game.toMove(), game.diceRemaining(), ui);
    }
}
