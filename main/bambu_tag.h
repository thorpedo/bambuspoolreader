#pragma once

#include <stdbool.h>
#include <stdint.h>

#define BAMBU_UID_LEN 4
#define BAMBU_SECTOR_COUNT 16
#define BAMBU_KEY_LEN 6
#define BAMBU_BLOCK_LEN 16

#define BAMBU_BLOCK_MATERIAL 2
#define BAMBU_BLOCK_NAME 4
#define BAMBU_BLOCK_COLOR 5

typedef struct {
    char material[BAMBU_BLOCK_LEN + 1];
    char name[BAMBU_BLOCK_LEN + 1];
    uint8_t r, g, b, a;
} bambu_tag_t;

// Layout: https://github.com/Bambu-Research-Group/RFID-Tag-Guide/blob/main/BambuLabRfid.md
bool bambu_tag_parse(const uint8_t *material_block, const uint8_t *name_block,
                     const uint8_t *color_block, bambu_tag_t *out);

// True when black text reads better than white on the spool color.
bool bambu_tag_needs_dark_text(const bambu_tag_t *tag);

// Derives the Key A of every sector from the tag UID.
bool bambu_derive_keys(const uint8_t uid[BAMBU_UID_LEN],
                       uint8_t keys[BAMBU_SECTOR_COUNT][BAMBU_KEY_LEN]);
