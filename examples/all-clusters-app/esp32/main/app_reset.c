#include <nvs_flash.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_log.h>
#include <esp32/rom/rtc.h>
#define RESET_COUNT 3

#define DEVICE_RESTART_TIMEOUT_MS                   (3000)
#define DEVICE_STORE_RESTART_COUNT_KEY              "restart_count"

static const char *TAG = "app_reset";
static void store_restart_count(uint8_t count)
{
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition("nvs", "restart_logic", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed");
        return;
    }
    err =  nvs_set_u8(handle, DEVICE_STORE_RESTART_COUNT_KEY, count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS set failed");
    }
    nvs_close(handle);
}

static uint8_t get_restart_count()
{
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition("nvs", "restart_logic", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed");
        return 0;
    }
    uint8_t restart_count = 0;
    err = nvs_get_u8(handle, DEVICE_STORE_RESTART_COUNT_KEY, &restart_count);
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "NVS get failed");
    }
    nvs_close(handle);
    return restart_count;
}
static void erase_restart_count()
{
    nvs_handle handle;
    esp_err_t err = nvs_open_from_partition("nvs", "restart_logic", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed");
        return;
    }
    err =  nvs_erase_key(handle, DEVICE_STORE_RESTART_COUNT_KEY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS erase failed");
    }
    nvs_close(handle);

}
void restart_count_erase_timercb(void *timer)
{
    if (!xTimerStop(timer, portMAX_DELAY)) {
        ESP_LOGE(TAG, "xTimerStop timer: %p", timer);
    }

    if (!xTimerDelete(timer, portMAX_DELAY)) {
        ESP_LOGE(TAG, "xTimerDelete timer: %p", timer);
    }

    erase_restart_count();
    ESP_LOGD(TAG, "Erase restart count");
}

int restart_count_get()
{
    static TimerHandle_t timer = NULL;
    RESET_REASON reset_reason  = rtc_get_reset_reason(0);
    uint8_t restart_count = get_restart_count();

    if (timer) {
        return restart_count;
    }

    /**< If the device restarts within the instruction time,
         the event_mdoe value will be incremented by one */
    if (reset_reason == POWERON_RESET || reset_reason == RTCWDT_RTC_RESET) {
        restart_count++;
        ESP_LOGD(TAG, "restart count: %d", restart_count);
    } else {
        restart_count = 1;
        ESP_LOGW(TAG, "restart reason: %d", reset_reason);
    }

    store_restart_count(restart_count);

    timer = xTimerCreate("restart_count_erase", DEVICE_RESTART_TIMEOUT_MS / portTICK_RATE_MS,
                         false, NULL, restart_count_erase_timercb);

    xTimerStart(timer, 0);

    return restart_count;
}
