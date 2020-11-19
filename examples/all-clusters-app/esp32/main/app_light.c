#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <light_driver.h>

#define DEFAULT_RED     255
#define DEFAULT_GREEN   169
#define DEFAULT_BLUE    87

static const char *TAG = "app_light";

uint8_t g_red = DEFAULT_RED;
uint8_t g_green = DEFAULT_GREEN;
uint8_t g_blue = DEFAULT_BLUE;
bool g_switch = false;

// BLUE
void app_light_indicate_bootup(void)
{
    light_driver_breath_start(0, 0, 255);
    vTaskDelay(3000 / portTICK_RATE_MS);
    light_driver_breath_stop();
    light_driver_set_switch(g_switch);
    light_driver_set_rgb(g_red, g_green, g_blue);
}
// RED
void app_light_indicate_wifi_fail(void)
{
    light_driver_breath_start(255, 0, 0);
    vTaskDelay(3000 / portTICK_RATE_MS);
    light_driver_breath_stop();
    light_driver_set_switch(g_switch);
    light_driver_set_rgb(g_red, g_green, g_blue);
}
// CYAN
void app_light_indicate_ota_start(void)
{
    light_driver_breath_start(0, 255, 255);
    vTaskDelay(3000 / portTICK_RATE_MS);
    light_driver_breath_stop();
    light_driver_set_switch(g_switch);
    light_driver_set_rgb(255, 169, 87);
}
// GREEN
void app_light_indicate_ota_success(void)
{
    light_driver_breath_start(0, 255, 0);
    vTaskDelay(3000 / portTICK_RATE_MS);
    light_driver_breath_stop();
    light_driver_set_switch(g_switch);
    light_driver_set_rgb(g_red, g_green, g_blue);
}
// ORANGE
void app_light_indicate_ota_fail(void)
{
    light_driver_breath_start(255, 69, 0);
    vTaskDelay(3000 / portTICK_RATE_MS);
    light_driver_breath_stop();
    light_driver_set_switch(g_switch);
    light_driver_set_rgb(g_red, g_green, g_blue);
}

// PALE YELLOW
void app_light_indicate_developer(void)
{
    light_driver_breath_start(128, 128, 0);
}

esp_err_t app_light_init(void)
{
    /**
     * NOTE:
     *  If the module has SPI flash, GPIOs 6-11 are connected to the module’s integrated SPI flash and PSRAM.
     *  If the module has PSRAM, GPIOs 16 and 17 are connected to the module’s integrated PSRAM.
     */
    light_driver_config_t driver_config = {
        .gpio_red        = CONFIG_LIGHT_GPIO_RED,
        .gpio_green      = CONFIG_LIGHT_GPIO_GREEN,
        .gpio_blue       = CONFIG_LIGHT_GPIO_BLUE,
        .gpio_cold       = CONFIG_LIGHT_GPIO_COLD,
        .gpio_warm       = CONFIG_LIGHT_GPIO_WARM,
        .fade_period_ms  = CONFIG_LIGHT_FADE_PERIOD_MS,
        .blink_period_ms = CONFIG_LIGHT_BLINK_PERIOD_MS,
    };
    /**
     * @brief Light driver initialization
     */
    ESP_ERROR_CHECK(light_driver_init(&driver_config));
    light_driver_set_switch(g_switch);
    light_driver_set_brightness(100);
    light_driver_set_rgb(0, 0, 0);
    return ESP_OK;
}
esp_err_t app_light_set_switch(bool switch_state)
{
    g_switch = switch_state;
    if (g_switch) {
        light_driver_set_rgb(g_red, g_green, g_blue);
    }
    light_driver_set_switch(g_switch);
    return ESP_OK;
}

esp_err_t app_light_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    g_red = red;
    g_green = green;
    g_blue = blue;
    light_driver_set_rgb(g_red, g_green, g_blue);
    return ESP_OK;
}
