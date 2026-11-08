// Host tests for the hardware-independent logic. See CLAUDE.md for how to run them.
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bambu_tag.h"
#include "battery_level.h"

static void test_parses_tag(void)
{
    uint8_t material[16] = "PLA";
    uint8_t name[16] = "PLA Basic";
    uint8_t color[16] = {0xFF, 0x6A, 0x13, 0xFF, 0xE8, 0x03};
    bambu_tag_t tag;

    assert(bambu_tag_parse(material, name, color, &tag));
    assert(strcmp(tag.material, "PLA") == 0);
    assert(strcmp(tag.name, "PLA Basic") == 0);
    assert(tag.r == 0xFF && tag.g == 0x6A && tag.b == 0x13 && tag.a == 0xFF);
}

static void test_full_block_string_is_terminated(void)
{
    uint8_t name[17] = "0123456789ABCDEF";
    uint8_t color[16] = {0};
    bambu_tag_t tag;

    assert(bambu_tag_parse(name, name, color, &tag));
    assert(strcmp(tag.name, "0123456789ABCDEF") == 0);
}

static void test_rejects_empty_or_binary_material(void)
{
    uint8_t empty[16] = {0};
    uint8_t binary[16] = {0x01, 0xFF};
    bambu_tag_t tag;

    assert(!bambu_tag_parse(empty, empty, empty, &tag));
    assert(!bambu_tag_parse(binary, empty, empty, &tag));
}

static void test_text_contrast(void)
{
    bambu_tag_t white = {.r = 0xFF, .g = 0xFF, .b = 0xFF};
    bambu_tag_t yellow = {.r = 0xF4, .g = 0xEE, .b = 0x2A};
    bambu_tag_t black = {.r = 0x00, .g = 0x00, .b = 0x00};
    bambu_tag_t blue = {.r = 0x0A, .g = 0x29, .b = 0x89};

    assert(bambu_tag_needs_dark_text(&white));
    assert(bambu_tag_needs_dark_text(&yellow));
    assert(!bambu_tag_needs_dark_text(&black));
    assert(!bambu_tag_needs_dark_text(&blue));
}

static void test_battery_percent(void)
{
    assert(battery_percent_from_mv(3000) == 0);
    assert(battery_percent_from_mv(3300) == 0);
    assert(battery_percent_from_mv(3750) == 50);
    assert(battery_percent_from_mv(4200) == 100);
    assert(battery_percent_from_mv(5000) == 100);
}

int main(void)
{
    test_parses_tag();
    test_full_block_string_is_terminated();
    test_rejects_empty_or_binary_material();
    test_text_contrast();
    test_battery_percent();
    puts("ok");
    return 0;
}
