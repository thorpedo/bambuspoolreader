#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool pn532_init(void);
// Returns true when a tag is present and writes its 4-byte UID.
bool pn532_find_tag(uint8_t uid[4]);
bool pn532_auth_key_a(uint8_t block, const uint8_t key[6], const uint8_t uid[4]);
bool pn532_read_block(uint8_t block, uint8_t out[16]);
