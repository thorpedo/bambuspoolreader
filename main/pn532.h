#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PN532_OK,
    PN532_ERR_BUS,           // the I2C driver could not be created
    PN532_ERR_LINE_LOW,      // SDA or SCL held low: short to GND or swapped wiring
    PN532_ERR_NO_PULLUP,     // nothing pulls the lines up: not wired or module unpowered
    PN532_ERR_NO_DEVICE,     // lines are pulled up but no device answers: module not in I2C mode
    PN532_ERR_WRONG_ADDRESS, // something answers, but not at the PN532 address
    PN532_ERR_NO_RESPONSE,   // the PN532 answers its address but not the commands
} pn532_status_t;

typedef enum {
    PN532_LINE_PULLED_UP,
    PN532_LINE_FLOATING,
    PN532_LINE_LOW,
} pn532_line_t;

// Logs a step-by-step diagnosis to the serial port when it fails.
pn532_status_t pn532_init(void);
// Returns true when a tag is present and writes its 4-byte UID.
bool pn532_find_tag(uint8_t uid[4]);
bool pn532_auth_key_a(uint8_t block, const uint8_t key[6], const uint8_t uid[4]);
bool pn532_read_block(uint8_t block, uint8_t out[16]);
