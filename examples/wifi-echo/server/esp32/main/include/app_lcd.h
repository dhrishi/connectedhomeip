/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef _APP_LCD_H_
#define _APP_LCD_H_
#include <stdint.h>
#include <stdbool.h>

void app_lcd_init();
esp_err_t app_lcd_display_qr_code(uint8_t *string);

#endif
