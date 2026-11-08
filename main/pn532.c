#include "pn532.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"

#define PN532_I2C_ADDR 0x24
#define PN532_I2C_HZ 100000
#define PN532_TIMEOUT_MS 1000
#define PN532_INIT_RETRIES 5
#define PN532_INIT_RETRY_MS 500

#define TFI_HOST 0xD4
#define TFI_PN532 0xD5

#define CMD_GET_FIRMWARE_VERSION 0x02
#define CMD_SAM_CONFIGURATION 0x14
#define CMD_RF_CONFIGURATION 0x32
#define CMD_IN_DATA_EXCHANGE 0x40
#define CMD_IN_LIST_PASSIVE_TARGET 0x4A

#define MIFARE_AUTH_A 0x60
#define MIFARE_READ 0x30

#define MAX_FRAME 64
#define PROBE_TIMEOUT_MS 50
#define RESPONSE_DATA_OFFSET 8 // status, preamble (3), LEN, LCS, TFI, response code

static const char *TAG = "pn532";

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_dev;

static void log_bus_scan(void)
{
    int found = 0;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_master_probe(s_bus, addr, PROBE_TIMEOUT_MS) == ESP_OK) {
            ESP_LOGI(TAG, "I2C device at 0x%02X", addr);
            found++;
        }
    }
    ESP_LOGI(TAG, "I2C scan: %d device(s), PN532 expected at 0x%02X", found, PN532_I2C_ADDR);
}

static bool wait_ready(void)
{
    uint8_t status = 0;
    for (int elapsed = 0; elapsed < PN532_TIMEOUT_MS; elapsed += 10) {
        if (i2c_master_receive(s_dev, &status, 1, PN532_TIMEOUT_MS) == ESP_OK && status == 0x01) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    return false;
}

static bool command(uint8_t cmd, const uint8_t *params, size_t params_len, uint8_t *resp,
                    size_t resp_cap, size_t *resp_len)
{
    static const uint8_t ACK[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
    uint8_t frame[MAX_FRAME] = {0x00, 0x00, 0xFF};
    size_t len = params_len + 2;

    if (len + 7 > sizeof(frame)) {
        return false;
    }
    frame[3] = (uint8_t)len;
    frame[4] = (uint8_t)(0x100 - len);
    frame[5] = TFI_HOST;
    frame[6] = cmd;
    memcpy(&frame[7], params, params_len);

    uint8_t sum = TFI_HOST + cmd;
    for (size_t i = 0; i < params_len; i++) {
        sum += params[i];
    }
    frame[7 + params_len] = (uint8_t)(0x100 - sum);
    frame[8 + params_len] = 0x00;

    if (i2c_master_transmit(s_dev, frame, 9 + params_len, PN532_TIMEOUT_MS) != ESP_OK) {
        return false;
    }

    uint8_t ack[1 + sizeof(ACK)];
    if (!wait_ready() || i2c_master_receive(s_dev, ack, sizeof(ack), PN532_TIMEOUT_MS) != ESP_OK ||
        memcmp(&ack[1], ACK, sizeof(ACK)) != 0) {
        return false;
    }

    uint8_t in[MAX_FRAME];
    if (!wait_ready() || i2c_master_receive(s_dev, in, sizeof(in), PN532_TIMEOUT_MS) != ESP_OK) {
        return false;
    }
    uint8_t in_len = in[4];
    if (in[1] != 0x00 || in[2] != 0x00 || in[3] != 0xFF || (uint8_t)(in_len + in[5]) != 0 ||
        in_len < 2 || in_len + 7 > sizeof(in) || in[6] != TFI_PN532 || in[7] != cmd + 1) {
        return false;
    }

    size_t data_len = in_len - 2;
    if (data_len > resp_cap) {
        return false;
    }
    memcpy(resp, &in[RESPONSE_DATA_OFFSET], data_len);
    *resp_len = data_len;
    return true;
}

static bool configure(void)
{
    uint8_t resp[8];
    size_t resp_len;
    static const uint8_t SAM_NORMAL[] = {0x01, 0x14, 0x01};
    // Retry only a few times so that polling without a tag returns quickly.
    static const uint8_t MAX_RETRIES[] = {0x05, 0xFF, 0x01, 0x10};

    return command(CMD_GET_FIRMWARE_VERSION, NULL, 0, resp, sizeof(resp), &resp_len) &&
           command(CMD_SAM_CONFIGURATION, SAM_NORMAL, sizeof(SAM_NORMAL), resp, sizeof(resp),
                   &resp_len) &&
           command(CMD_RF_CONFIGURATION, MAX_RETRIES, sizeof(MAX_RETRIES), resp, sizeof(resp),
                   &resp_len);
}

bool pn532_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_NFC_SDA,
        .scl_io_num = PIN_NFC_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (i2c_new_master_bus(&bus_cfg, &s_bus) != ESP_OK) {
        return false;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PN532_I2C_ADDR,
        .scl_speed_hz = PN532_I2C_HZ,
    };
    if (i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev) != ESP_OK) {
        return false;
    }

    for (int retry = 0; retry <= PN532_INIT_RETRIES; retry++) {
        if (retry > 0) {
            ESP_LOGW(TAG, "Init failed, retry %d of %d", retry, PN532_INIT_RETRIES);
            vTaskDelay(pdMS_TO_TICKS(PN532_INIT_RETRY_MS));
        }
        if (configure()) {
            return true;
        }
    }

    log_bus_scan();
    return false;
}

bool pn532_find_tag(uint8_t uid[4])
{
    static const uint8_t ONE_ISO14443A[] = {0x01, 0x00};
    uint8_t resp[32];
    size_t resp_len;

    if (!command(CMD_IN_LIST_PASSIVE_TARGET, ONE_ISO14443A, sizeof(ONE_ISO14443A), resp,
                 sizeof(resp), &resp_len)) {
        return false;
    }
    // NbTg, Tg, SENS_RES (2), SEL_RES, NFCIDLength, NFCID...
    if (resp_len < 10 || resp[0] != 1 || resp[5] != 4) {
        return false;
    }
    memcpy(uid, &resp[6], 4);
    return true;
}

bool pn532_auth_key_a(uint8_t block, const uint8_t key[6], const uint8_t uid[4])
{
    uint8_t params[13] = {0x01, MIFARE_AUTH_A, block};
    uint8_t resp[1];
    size_t resp_len;

    memcpy(&params[3], key, 6);
    memcpy(&params[9], uid, 4);
    return command(CMD_IN_DATA_EXCHANGE, params, sizeof(params), resp, sizeof(resp), &resp_len) &&
           resp_len == 1 && resp[0] == 0x00;
}

bool pn532_read_block(uint8_t block, uint8_t out[16])
{
    const uint8_t params[] = {0x01, MIFARE_READ, block};
    uint8_t resp[17];
    size_t resp_len;

    if (!command(CMD_IN_DATA_EXCHANGE, params, sizeof(params), resp, sizeof(resp), &resp_len) ||
        resp_len != sizeof(resp) || resp[0] != 0x00) {
        return false;
    }
    memcpy(out, &resp[1], 16);
    return true;
}
