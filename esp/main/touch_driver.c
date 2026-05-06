#include "touch_driver.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_log.h"
#include "lvgl.h"

#define TOUCH_CS_PIN   33
#define TOUCH_IRQ_PIN  36

#define SCREEN_W 320
#define SCREEN_H 240

static const char *TAG = "touch";

static esp_lcd_touch_handle_t tp = NULL;

static void touch_driver_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    esp_lcd_touch_point_data_t tp_data[1];
    uint8_t count = 0;

    ESP_ERROR_CHECK(esp_lcd_touch_read_data(tp));

    data->state = LV_INDEV_STATE_REL;

    if (esp_lcd_touch_get_data(tp, tp_data, &count, 1) == ESP_OK && count > 0) {
        data->point.x = tp_data[0].x;
        data->point.y = tp_data[0].y;
        data->state = LV_INDEV_STATE_PR;
    }

    data->continue_reading = false;
}

esp_err_t touch_driver_init(void) {
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(TOUCH_CS_PIN);

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)SPI3_HOST, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = SCREEN_W,
        .y_max = SCREEN_H,
        .rst_gpio_num = -1,
        .int_gpio_num = TOUCH_IRQ_PIN,
        .flags = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = false,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller XPT2046");
    ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_driver_read;
    lv_indev_drv_register(&indev_drv);

    return ESP_OK;
}
