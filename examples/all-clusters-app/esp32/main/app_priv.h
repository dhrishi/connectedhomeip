/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once
#include "stdbool.h"
#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICE_KEY_ADDR_OFFSET  0
#define DEVICE_CERT_ADDR_OFFSET (1 * 4096)
#define CA_CERT_ADDR_OFFSET     (2 * 4096)
#define MQTT_INFO_ADDR_OFFSET   (3 * 4096)

extern uint8_t g_red, g_green, g_blue;

int restart_count_get();

void app_light_indicate_bootup(void);
void app_light_indicate_wifi_fail(void);
void app_light_indicate_ota_start(void);
void app_light_indicate_ota_success(void);
void app_light_indicate_ota_fail(void);
void app_light_indicate_developer(void);

esp_err_t app_light_init(void);
esp_err_t app_light_set_switch(bool switch_state);
esp_err_t app_reboot_timer_create(void);
esp_err_t app_reboot_timer_start(uint32_t msec);
esp_err_t app_light_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

esp_err_t do_firmware_upgrade(const char *url);
httpd_handle_t start_webserver(void);

#ifdef __cplusplus
}
#endif

