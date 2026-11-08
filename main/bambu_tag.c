#include "bambu_tag.h"

static void copy_string(char *dst, const uint8_t *block)
{
    int i = 0;
    for (; i < BAMBU_BLOCK_LEN && block[i] >= 0x20 && block[i] < 0x7f; i++) {
        dst[i] = (char)block[i];
    }
    dst[i] = '\0';
}

bool bambu_tag_parse(const uint8_t *material_block, const uint8_t *name_block,
                     const uint8_t *color_block, bambu_tag_t *out)
{
    copy_string(out->material, material_block);
    copy_string(out->name, name_block);
    out->r = color_block[0];
    out->g = color_block[1];
    out->b = color_block[2];
    out->a = color_block[3];
    return out->material[0] != '\0';
}

bool bambu_tag_needs_dark_text(const bambu_tag_t *tag)
{
    int luma = (299 * tag->r + 587 * tag->g + 114 * tag->b) / 1000;
    return luma > 140;
}
