#include "battery.h"

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"

#define BATTERY_ADC_CHANNEL ADC_CHANNEL_6 // GPIO34
#define BATTERY_ADC_ATTEN ADC_ATTEN_DB_12
#define BATTERY_DIVIDER 2
#define BATTERY_SAMPLES 16
#define BATTERY_SETTLE_MS 10

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;

void battery_init(void)
{
    gpio_set_direction(PIN_BATTERY_ADC_EN, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BATTERY_ADC_EN, 0);

    adc_oneshot_unit_init_cfg_t unit_cfg = {.unit_id = ADC_UNIT_1};
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, BATTERY_ADC_CHANNEL, &chan_cfg));

    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali));
}

int battery_read_mv(void)
{
    // The divider drains the battery, so it is only enabled while measuring.
    gpio_set_level(PIN_BATTERY_ADC_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(BATTERY_SETTLE_MS));

    int total = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
        int raw = 0;
        int mv = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc, BATTERY_ADC_CHANNEL, &raw));
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(s_cali, raw, &mv));
        total += mv;
    }
    gpio_set_level(PIN_BATTERY_ADC_EN, 0);
    return total / BATTERY_SAMPLES * BATTERY_DIVIDER;
}
