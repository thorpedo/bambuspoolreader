#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bambu_tag.h"
#include "battery.h"
#include "display.h"
#include "pn532.h"

#define POLL_INTERVAL_MS 500
#define BATTERY_INTERVAL_MS 60000
#define BATTERY_TASK_STACK 4096
#define SECTOR_BLOCKS 4
#define DEMO_INTERVAL_MS 3000

static const char *TAG = "app";

static bool read_block(const uint8_t uid[BAMBU_UID_LEN],
                       uint8_t keys[BAMBU_SECTOR_COUNT][BAMBU_KEY_LEN], uint8_t block,
                       uint8_t *out)
{
    return pn532_auth_key_a(block, keys[block / SECTOR_BLOCKS], uid) &&
           pn532_read_block(block, out);
}

static bool read_tag(const uint8_t uid[BAMBU_UID_LEN], bambu_tag_t *tag)
{
    uint8_t keys[BAMBU_SECTOR_COUNT][BAMBU_KEY_LEN];
    uint8_t material[BAMBU_BLOCK_LEN];
    uint8_t name[BAMBU_BLOCK_LEN];
    uint8_t color[BAMBU_BLOCK_LEN];

    return bambu_derive_keys(uid, keys) &&
           read_block(uid, keys, BAMBU_BLOCK_MATERIAL, material) &&
           read_block(uid, keys, BAMBU_BLOCK_NAME, name) &&
           read_block(uid, keys, BAMBU_BLOCK_COLOR, color) &&
           bambu_tag_parse(material, name, color, tag);
}

// Runs apart from the NFC loop so the indicator keeps updating without a reader.
static void battery_task(void *arg)
{
    for (;;) {
        int mv = battery_read_mv();
        ESP_LOGI(TAG, "Battery: %d mV", mv);
        display_show_battery(mv);
        vTaskDelay(pdMS_TO_TICKS(BATTERY_INTERVAL_MS));
    }
}

#ifdef DEMO_TAGS
// Cycles through sample spools to check the screen without a reader. The data is made up.
static void run_demo(void)
{
    static const bambu_tag_t SAMPLES[] = {
        {.material = "PLA", .name = "PLA Basic", .r = 0xFF, .g = 0x6A, .b = 0x13},
        {.material = "PETG", .name = "PETG Basic", .r = 0xFF, .g = 0xFF, .b = 0xFF},
        {.material = "PLA", .name = "Support for PLA", .r = 0x0A, .g = 0x29, .b = 0x89},
        {.material = "PLA-CF", .name = "PLA-CF", .r = 0x00, .g = 0x00, .b = 0x00},
        {.material = "PLA", .name = "PLA Silk", .r = 0xF4, .g = 0xEE, .b = 0x2A},
    };

    for (int i = 0;; i = (i + 1) % (sizeof(SAMPLES) / sizeof(SAMPLES[0]))) {
        ESP_LOGI(TAG, "Demo: %s / %s", SAMPLES[i].material, SAMPLES[i].name);
        display_show_tag(&SAMPLES[i]);
        vTaskDelay(pdMS_TO_TICKS(DEMO_INTERVAL_MS));
    }
}
#endif

void app_main(void)
{
    display_init();
    battery_init();
    xTaskCreate(battery_task, "battery", BATTERY_TASK_STACK, NULL, tskIDLE_PRIORITY + 1, NULL);

#ifdef DEMO_TAGS
    run_demo();
#endif

    if (!pn532_init()) {
        ESP_LOGE(TAG, "PN532 not found");
        display_show_message("Sin lector NFC");
        return;
    }
    display_show_message("Acerca una bobina");

    uint8_t last_uid[BAMBU_UID_LEN] = {0};
    bool tag_present = false;

    for (;;) {
        uint8_t uid[BAMBU_UID_LEN];

        if (!pn532_find_tag(uid)) {
            tag_present = false;
        } else if (!tag_present || memcmp(uid, last_uid, sizeof(uid)) != 0) {
            bambu_tag_t tag;

            tag_present = true;
            memcpy(last_uid, uid, sizeof(uid));
            if (read_tag(uid, &tag)) {
                display_show_tag(&tag);
            } else {
                ESP_LOGW(TAG, "Could not read tag");
                display_show_message("Etiqueta no valida");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}
