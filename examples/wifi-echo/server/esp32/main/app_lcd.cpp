/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "qrcodegen.h"

/*SPI & LCD Driver Includes*/
#include "iot_lcd.h"
#include "FreeSerifItalic24pt7b.h"
#include "FreeSerifItalic9pt7b.h"
#include "image.h"

CEspLcd* tft = NULL;
void app_lcd_init()
{
    /*ESP Wrover kit config*/
    lcd_conf_t lcd_pins = {
        .lcd_model = LCD_MOD_ST7789,
        .pin_num_miso = GPIO_NUM_25,
        .pin_num_mosi = GPIO_NUM_23,
        .pin_num_clk = GPIO_NUM_19,
        .pin_num_cs = GPIO_NUM_22,
        .pin_num_dc = GPIO_NUM_21,
        .pin_num_rst = GPIO_NUM_18,
        .pin_num_bckl = GPIO_NUM_5,
        .clk_freq = 20 * 1000 * 1000,
        .rst_active_level = 0,
        .bckl_active_level = 0,
        .spi_host = HSPI_HOST,
        .init_spi_bus = true,
    };

    /*Initialize SPI Handler*/
    if (tft == NULL) {
        tft = new CEspLcd(&lcd_pins);
    }

    /*screen initialize*/
    tft->invertDisplay(false);
    tft->setRotation(1);             //Landscape mode
    tft->fillScreen(COLOR_ESP_BKGD);
    tft->drawBitmap(0, 0, esp_logo, 137, 26);
    //    tft->drawBitmap(243, 0, aws_logo, 77, 31);
    tft->setFont(&FreeSerifItalic24pt7b);
    tft->fillRect(0, 30, 230, 10, COLOR_ESP_BKGD);
    tft->drawString("Initialising...", 5, 90);
    tft->setFont(&FreeSerifItalic9pt7b);
    tft->drawString("Powered by ESP RainMaker", 110, 230);
}


static const char *lt[] = {
    /* 0 */ "  ",
    /* 1 */ "\u2580 ",
    /* 2 */ " \u2580",
    /* 3 */ "\u2580\u2580",
    /* 4 */ "\u2584 ",
    /* 5 */ "\u2588 ",
    /* 6 */ "\u2584\u2580",
    /* 7 */ "\u2588\u2580",
    /* 8 */ " \u2584",
    /* 9 */ "\u2580\u2584",
    /* 10 */ " \u2588",
    /* 11 */ "\u2580\u2588",
    /* 12 */ "\u2584\u2584",
    /* 13 */ "\u2588\u2584",
    /* 14 */ "\u2584\u2588",
    /* 15 */ "\u2588\u2588",
};

void print_qr_char(unsigned char n)
{
    printf("%s", lt[n]);
}



esp_err_t app_lcd_display_qr_code(uint8_t *data)
{
    if (!tft) {
        return ESP_FAIL;
    }
    const int kBorderSize = 1, kModuleSize = 4;
    const int DisplayWidth = 320, DisplayHeight = 240, ScreenTitleSafeTop = 45, ScreenTitleSafeBottom = 45;
    const int size        = qrcodegen_getSize(data);
    const int displaySize = (2 * kBorderSize + size) * kModuleSize;
    const int displayX    = (DisplayWidth - displaySize) / 2;
    const int displayY    = ScreenTitleSafeTop + ((DisplayHeight - ScreenTitleSafeTop - ScreenTitleSafeBottom) - displaySize) / 2;

    tft->fillScreen(COLOR_ESP_BKGD);

    tft->fillRect(displayX, displayY, displaySize, displaySize, COLOR_LIGHTGREY);

    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            if (qrcodegen_getModule(data, x, y))
            {
                tft->fillRect(displayX + (kBorderSize + x) * kModuleSize, displayY + (kBorderSize + y) * kModuleSize, kModuleSize,
                             kModuleSize, COLOR_BLACK);
            }
        }
    }

    return ESP_OK;
}
