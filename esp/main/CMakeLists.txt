idf_component_register(
    SRCS "main.c" "ui.c" "serial_parser.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd driver esp_lcd_ili9341 lvgl nvs_flash esp_driver_uart
)
