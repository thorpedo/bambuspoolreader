#include "display.h"

#include <stdio.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lvgl_port.h"

#include "battery_level.h"
#include "board.h"

#define LCD_HOST SPI2_HOST
#define LCD_H_RES 240
#define LCD_V_RES 135
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)
// The 135x240 panel sits in the middle of the controller's 240x320 RAM.
#define LCD_GAP_X 40
#define LCD_GAP_Y 53
#define LCD_DRAW_LINES 20
#define PANEL_WIDTH (LCD_H_RES / 2)
#define MARGIN 8
#define NAME_MARGIN (MARGIN + 2)
#define NAME_X (PANEL_WIDTH + NAME_MARGIN)
#define NAME_WIDTH (LCD_H_RES - NAME_X - NAME_MARGIN)
// Messages have no colour panel beside them, so they use the whole width.
#define MESSAGE_WIDTH (LCD_H_RES - 2 * NAME_MARGIN)
#define BATTERY_TEXT_LEN 16

// LVGL ships Montserrat only in medium weight: bold is faked by drawing the text twice.
typedef struct {
    lv_obj_t *back;
    lv_obj_t *front;
} bold_label_t;

static bold_label_t s_battery;
static bold_label_t s_material;
static bold_label_t s_name;
static lv_obj_t *s_swatch;

static void panel_init(esp_lcd_panel_io_handle_t *io, esp_lcd_panel_handle_t *panel)
{
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_LCD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * LCD_DRAW_LINES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, io));

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(*io, &panel_cfg, panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(*panel));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(*panel, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(*panel, LCD_GAP_X, LCD_GAP_Y));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(*panel, true));

    gpio_set_direction(PIN_LCD_BL, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LCD_BL, 1);
}

static lv_obj_t *add_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align, int x, int y,
                           int width)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_width(label, width);
    lv_obj_align(label, align, x, y);
    lv_label_set_text(label, "");
    return label;
}

static bold_label_t add_bold_label(lv_obj_t *parent, const lv_font_t *font, lv_align_t align,
                                   int x, int y, int width)
{
    bold_label_t label = {
        .back = add_label(parent, font, align, x, y, width),
        .front = add_label(parent, font, align, x + 1, y, width),
    };
    return label;
}

static void bold_label_set_text(const bold_label_t *label, const char *text)
{
    lv_label_set_text(label->back, text);
    lv_label_set_text(label->front, text);
}

static void bold_label_place(const bold_label_t *label, int x, int width)
{
    lv_obj_set_x(label->back, x);
    lv_obj_set_x(label->front, x + 1);
    lv_obj_set_width(label->back, width);
    lv_obj_set_width(label->front, width);
}

void display_init(void)
{
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_handle_t panel;
    panel_init(&io, &panel);

    const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io,
        .panel_handle = panel,
        .buffer_size = LCD_H_RES * LCD_DRAW_LINES,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .rotation = {.swap_xy = true, .mirror_x = true, .mirror_y = false},
        .flags = {.buff_dma = true, .swap_bytes = true},
    };
    lvgl_port_add_disp(&disp_cfg);

    lvgl_port_lock(0);
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    s_swatch = lv_obj_create(screen);
    lv_obj_set_size(s_swatch, PANEL_WIDTH, LCD_V_RES);
    lv_obj_set_pos(s_swatch, 0, 0);
    lv_obj_set_style_radius(s_swatch, 0, 0);
    lv_obj_set_style_border_width(s_swatch, 0, 0);
    lv_obj_set_style_pad_all(s_swatch, MARGIN / 2, 0);
    lv_obj_set_scrollbar_mode(s_swatch, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_hidden(s_swatch, true);

    s_material = add_bold_label(s_swatch, &lv_font_montserrat_28, LV_ALIGN_LEFT_MID, 0, 0,
                                PANEL_WIDTH - MARGIN - 1);
    lv_obj_set_style_text_align(s_material.back, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(s_material.front, LV_TEXT_ALIGN_CENTER, 0);

    s_name = add_bold_label(screen, &lv_font_montserrat_20, LV_ALIGN_TOP_LEFT, NAME_X, NAME_MARGIN,
                            NAME_WIDTH);
    s_battery = add_bold_label(screen, &lv_font_montserrat_14, LV_ALIGN_BOTTOM_RIGHT, -MARGIN - 1,
                               -MARGIN, LV_SIZE_CONTENT);
    lvgl_port_unlock();
}

void display_show_message(const char *message)
{
    lvgl_port_lock(0);
    bold_label_place(&s_name, NAME_MARGIN, MESSAGE_WIDTH);
    bold_label_set_text(&s_name, message);
    lv_obj_set_hidden(s_swatch, true);
    lvgl_port_unlock();
}

void display_show_tag(const bambu_tag_t *tag)
{
    lvgl_port_lock(0);
    lv_color_t text_color = bambu_tag_needs_dark_text(tag) ? lv_color_black() : lv_color_white();
    bold_label_set_text(&s_material, tag->material);
    lv_obj_set_style_text_color(s_material.back, text_color, 0);
    lv_obj_set_style_text_color(s_material.front, text_color, 0);
    bold_label_place(&s_name, NAME_X, NAME_WIDTH);
    bold_label_set_text(&s_name, tag->name);
    lv_obj_set_style_bg_color(s_swatch, lv_color_make(tag->r, tag->g, tag->b), 0);
    lv_obj_set_hidden(s_swatch, false);
    lvgl_port_unlock();
}

void display_show_battery(int mv)
{
    char text[BATTERY_TEXT_LEN];
    if (mv >= BATTERY_USB_MV) {
        snprintf(text, sizeof(text), LV_SYMBOL_USB " USB");
    } else {
        snprintf(text, sizeof(text), LV_SYMBOL_BATTERY_FULL " %d%%", battery_percent_from_mv(mv));
    }

    lvgl_port_lock(0);
    bold_label_set_text(&s_battery, text);
    lvgl_port_unlock();
}
