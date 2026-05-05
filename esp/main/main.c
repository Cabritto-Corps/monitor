#include "serial_parser.h"
#include "ui.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include <string.h>

/* ---- Pin definitions (matching working Arduino code) ---- */
#define PIN_MOSI     23
#define PIN_MISO     19
#define PIN_CLK      18
#define PIN_CS       15
#define PIN_DC       2
#define PIN_RST      4
#define PIN_BACKLIGHT 5

#define LCD_HOST     SPI3_HOST
#define LCD_PCLK_HZ  (20 * 1000 * 1000)

/* ---- UART for PC communication (UART0 = USB serial, matches Arduino Serial) ---- */
#define UART_PORT    UART_NUM_0
#define UART_TX      1
#define UART_RX      3
#define UART_BAUD    115200
#define UART_BUF_SZ  256

/* ---- Display ---- */
#define SCREEN_W     320
#define SCREEN_H     240
#define DRAW_BUF_LINES (SCREEN_H / 4)

static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;
static bool connected = false;
static int64_t last_data_time = 0;

/* ---- LVGL flush callback (RGB565 -> BGR565 conversion) ---- */
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area,
                          lv_color_t *color_map) {
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;

    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1,
                              area->x1 + w, area->y1 + h,
                              color_map);
    lv_disp_flush_ready(drv);
}

/* ---- Display init ---- */
static void display_init(void) {
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_CLK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SCREEN_W * DRAW_BUF_LINES * 2 + 8,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = PIN_DC,
        .cs_gpio_num = PIN_CS,
        .pclk_hz = LCD_PCLK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io_handle));

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(
        esp_lcd_new_panel_ili9341(io_handle, &panel_cfg, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));

    gpio_set_direction(PIN_BACKLIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BACKLIGHT, 1);

    uint16_t *test = heap_caps_malloc(SCREEN_W * SCREEN_H * 2, MALLOC_CAP_DMA);
    if (test) {
        memset(test, 0x00, SCREEN_W * SCREEN_H * 2);
        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, SCREEN_W, SCREEN_H, test);
        heap_caps_free(test);
    }
}

/* ---- UART init ---- */
static void uart_init(void) {
    uart_config_t uart_cfg = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_PORT, &uart_cfg);
    uart_set_pin(UART_PORT, UART_TX, UART_RX,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT, UART_BUF_SZ * 2,
                        UART_BUF_SZ * 2, 0, NULL, 0);
}

/* ---- LVGL init ---- */
static void lvgl_init(void) {
    lv_init();

    int buf_pixels = SCREEN_W * DRAW_BUF_LINES;
    buf1 = heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);
    buf2 = heap_caps_malloc(buf_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 && buf2);

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buf_pixels);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_W;
    disp_drv.ver_res = SCREEN_H;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

/* ---- UART reader task ---- */
static void uart_task(void *arg) {
    (void)arg;
    char buf[UART_BUF_SZ + 1];
    int buf_idx = 0;

    while (1) {
        uint8_t ch;
        int len = uart_read_bytes(UART_PORT, &ch, 1,
                                  pdMS_TO_TICKS(20));
        if (len > 0) {
            if (ch == '\n' || ch == '\r') {
                if (buf_idx > 0) {
                    buf[buf_idx] = '\0';
                    MonitorData data;
                    if (serial_parser_parse(buf, &data) == 0) {
                        const char *names[MAX_PROCESSES];
                        const char *rams[MAX_PROCESSES];
                        int n = data.process_count;
                        for (int i = 0; i < n; i++) {
                            names[i] = data.processes[i].name;
                            rams[i] = data.processes[i].ram_mb;
                        }
                        ui_update_metrics(data.cpu_pct, data.ram_pct,
                                          names, rams, n);
                        connected = true;
                        last_data_time = esp_timer_get_time();
                    }
                    buf_idx = 0;
                }
            } else if (buf_idx < UART_BUF_SZ) {
                buf[buf_idx++] = (char)ch;
            }
        }

        if (connected &&
            esp_timer_get_time() - last_data_time > 5000000LL) {
            connected = false;
            ui_show_disconnected();
        }

        vTaskDelay(1);
    }
}

void app_main(void) {
    nvs_flash_init();
    display_init();
    uart_init();
    lvgl_init();

    ui_init();

    xTaskCreate(uart_task, "uart", 4096, NULL, 5, NULL);

    last_data_time = esp_timer_get_time();

    while (1) {
        lv_tick_inc(1);
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
