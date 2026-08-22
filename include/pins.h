#pragma once

// I2C (OLED) uses the board's default SDA/SCL (A4/A5) — no need to set pins
// explicitly, Wire.begin() picks them up from the Nano ESP32 variant.

// Rotary encoder
static constexpr uint8_t PIN_ENCODER_CLK = D2;
static constexpr uint8_t PIN_ENCODER_DT  = D3;
static constexpr uint8_t PIN_ENCODER_SW  = D4;

// SSD1306 display
static constexpr uint8_t SCREEN_WIDTH  = 128;
static constexpr uint8_t SCREEN_HEIGHT = 64;
static constexpr uint8_t SCREEN_I2C_ADDR = 0x3C; // most common; try 0x3D if blank
