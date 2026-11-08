#pragma once

// LilyGO T-Display (ESP32). Display and battery pins come from the vendor's factory test.
#define PIN_LCD_MOSI 19
#define PIN_LCD_SCLK 18
#define PIN_LCD_CS 5
#define PIN_LCD_DC 16
#define PIN_LCD_RST 23
#define PIN_LCD_BL 4

#define PIN_BATTERY_ADC_EN 14

// PN532 wiring, chosen by this project: both pins are free on the board.
#define PIN_NFC_SDA 21
#define PIN_NFC_SCL 22
