#pragma once

// Rotary encoder
static constexpr uint8_t PIN_ENCODER_CLK = D2;
static constexpr uint8_t PIN_ENCODER_DT  = D3;
static constexpr uint8_t PIN_ENCODER_SW  = D4;

// Dedicated "menu/quit" button — wired to GND, same as the encoder's switch.
static constexpr uint8_t PIN_MENU_BUTTON = D5;

// ILI9341 TFT (SPI). MOSI/MISO/SCK use the board's default hardware SPI
// pins (D11/D12/D13) — Adafruit_ILI9341's constructor uses the default SPI
// bus implicitly, so only the control pins need to be named here.
static constexpr uint8_t PIN_TFT_CS  = D10;
static constexpr uint8_t PIN_TFT_DC  = D9;
static constexpr uint8_t PIN_TFT_RST = D8;

// Panel is 240x320 native (portrait); we run it rotated to landscape.
static constexpr int SCREEN_WIDTH  = 320;
static constexpr int SCREEN_HEIGHT = 240;
