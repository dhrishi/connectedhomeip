#include <esp_http_server.h>
#include <sys/param.h>
#include <esp_log.h>
#include <json_parser.h>
#include <esp_timer.h>
#include <esp_partition.h>
#include <mdf_info_store.h>

#include <light_driver.h>
#include <light_handle.h>
#include "app_priv.h"

#define DEVICE_CERT "dev-cert"
#define DEVICE_KEY  "dev-key"
#define CA_CERT     "ca-cert"
#define MQTT_INFO   "mqtt-info"

static const char *TAG = "app_httpd";
esp_timer_handle_t ota_timer;

#define SECTOR_SIZE (4 * 1024)

static esp_err_t app_httpd_report_error(httpd_req_t *req, char *status, char *description)
{
    char buf[100];
    snprintf(buf, sizeof(buf), "{\"status\":\"%s\",\"description\":\"%s\"}", status, description);
    return httpd_resp_send(req, buf, strlen(buf));
}

esp_err_t app_reboot_timer_start(uint32_t msec)
{
    return esp_timer_start_once(ota_timer, msec * 1000U);
}

static esp_err_t ota_post_handler(httpd_req_t *req)
{
    char buf[100] = {0};
    int ret, remaining = req->content_len;

    ESP_LOGI(TAG, "Received %d bytes on %s", req->content_len, req->uri);
    /* Read the data for the request */
    if ((ret = httpd_req_recv(req, buf,
                    MIN(remaining, sizeof(buf)))) <= 0) {
        return app_httpd_report_error(req, "failure", "httpd read failed");
    }
    ESP_LOGI(TAG, "Got firmware upgrade url %s", buf);
    app_light_indicate_ota_start();
    esp_err_t err = do_firmware_upgrade(buf);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Firmware Upgrade Successful");
        app_light_indicate_ota_success();
        app_reboot_timer_start(5000);
        return app_httpd_report_error(req, "success", "FW upgrade successful");
    } else {
        app_light_indicate_ota_fail();
        ESP_LOGE(TAG, "Firmware Upgrade Failed");
        return app_httpd_report_error(req, "failure", "FW upgrade failed");
    }
    return ESP_OK;
}

static httpd_uri_t ota_handler = {
    .uri       = "/ota",
    .method    = HTTP_POST,
    .handler   = ota_post_handler,
    .user_ctx  = NULL
};
#if 0
static esp_err_t light_set_post_handler(httpd_req_t *req)
{
    char buf[100] = {0};
    int ret, remaining = req->content_len;

    ESP_LOGI(TAG, "Received %d bytes on %s", req->content_len, req->uri);
    /* Read the data for the request */
    if ((ret = httpd_req_recv(req, buf,
                    MIN(remaining, sizeof(buf)))) <= 0) {
        return app_httpd_report_error(req, "failure", "httpd read failed");
    }
    ESP_LOGI(TAG, "Got data %s %d", buf, ret);
    jparse_ctx_t jctx;
    if (json_parse_start(&jctx, buf, ret) != 0) {
        return app_httpd_report_error(req, "failure", "JSON parse failed");
    }
    int cid = 0, val = 0;
    esp_err_t err = ESP_FAIL;
    if ((json_obj_get_int(&jctx, "cid", &cid) == 0 ) &&
            (json_obj_get_int(&jctx, "val", &val) == 0 )) {
        ESP_LOGI(TAG, "Setting value");
        err = led_light_set_value(cid, val);
    } else {
        json_parse_end(&jctx);
        return app_httpd_report_error(req, "failure", "JSON keys not found");
    }
    json_parse_end(&jctx);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Light changed successfully");
            return app_httpd_report_error(req, "success", "Light state changed");
    } else {
        ESP_LOGE(TAG, "Light change failed");
        return app_httpd_report_error(req, "failure", "Light state change failed");
    }
    return ESP_OK;
}

static httpd_uri_t light_set_handler = {
    .uri       = "/light-set",
    .method    = HTTP_POST,
    .handler   = light_set_post_handler,
    .user_ctx  = NULL
};

static esp_err_t light_get_post_handler(httpd_req_t *req)
{
    char buf[100] = {0};
    int ret, remaining = req->content_len;

    ESP_LOGI(TAG, "Received %d bytes on %s", req->content_len, req->uri);
    /* Read the data for the request */
    if ((ret = httpd_req_recv(req, buf,
                    MIN(remaining, sizeof(buf)))) <= 0) {
        return app_httpd_report_error(req, "failure", "httpd read failed");
    }
    ESP_LOGI(TAG, "Got data %s", buf);
    jparse_ctx_t jctx;
    if (json_parse_start(&jctx, buf, ret) != 0) {
        return app_httpd_report_error(req, "failure", "JSON parse failed");
    }
    int cid = 0, val = 0;
    esp_err_t err = ESP_FAIL;
    if (json_obj_get_int(&jctx, "cid", &cid) == 0 ) {
        err = led_light_get_value(cid, &val);
    } else {
        json_parse_end(&jctx);
        return app_httpd_report_error(req, "failure", "JSON keys not found");
    }
    json_parse_end(&jctx);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Light queried successfully");
        snprintf(buf, sizeof(buf), "{\"cid\":%d,\"val\":%d}", cid, val);
            httpd_resp_send(req, buf, strlen(buf));
    } else {
        ESP_LOGE(TAG, "Light query failed");
        return app_httpd_report_error(req, "failure", "Light query failed");
    }
    return ESP_OK;
}

static httpd_uri_t light_get_handler = {
    .uri       = "/light-get",
    .method    = HTTP_POST,
    .handler   = light_get_post_handler,
    .user_ctx  = NULL
};
#endif
static void handle_rgb(jparse_ctx_t *jctx)
{
    int r = 0, g = 0, b = 0;
    json_obj_get_int(jctx, "r", &r);
    json_obj_get_int(jctx, "g", &g);
    json_obj_get_int(jctx, "b", &b);

    ESP_LOGI(TAG, "R: %d, G: %d, B: %d", r, g, b);
    app_light_set_rgb(r, g, b);
}

esp_err_t esp_light_parse_command(char *data, size_t len)
{
    jparse_ctx_t jctx;
    if (json_parse_start(&jctx, data, len) == 0) {
        ESP_LOGI(TAG, "Checking RGB");
        if (json_obj_get_object(&jctx, "rgb") == 0) {
            handle_rgb(&jctx);
            json_obj_leave_object(&jctx);
        }
        bool output;
        ESP_LOGI(TAG, "Checking Output");
        if (json_obj_get_bool(&jctx, "output", &output) == 0) {
            app_light_set_switch(output);
            ESP_LOGI(TAG, "Changing state to %s", output ? "True" : "False");
        }
        bool save_default;
        ESP_LOGI(TAG, "Checking Default");
        if (json_obj_get_bool(&jctx, "default", &save_default) == 0) {
            ESP_LOGI(TAG, "Saving to NVS");
        }
        json_parse_end(&jctx);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "JSON Parse failed");
    }
    return ESP_FAIL;
}

static esp_err_t light_ctrl_post_handler(httpd_req_t *req)
{
    char buf[256] = {0};
    int ret, remaining = req->content_len;

    ESP_LOGI(TAG, "Received %d bytes on %s", req->content_len, req->uri);
    /* Read the data for the request */
    if ((ret = httpd_req_recv(req, buf,
                    MIN(remaining, sizeof(buf)))) <= 0) {
        return app_httpd_report_error(req, "failure", "httpd read failed");
    }
    ESP_LOGI(TAG, "Got data %s", buf);
    if (esp_light_parse_command(buf, ret) == ESP_OK) {
        ESP_LOGI(TAG, "Light changed successfully");
            return app_httpd_report_error(req, "success", "Light state changed");
    } else {
        ESP_LOGE(TAG, "Light change failed");
        return app_httpd_report_error(req, "failure", "Light state change failed");
    }
    return ESP_OK;
}

httpd_uri_t light_ctrl_handler = {
    .uri       = "/light-ctrl",
    .method    = HTTP_POST,
    .handler   = light_ctrl_post_handler,
    .user_ctx  = NULL
};

esp_err_t app_partition_iterate(httpd_req_t *req)
{
    esp_partition_iterator_t iterator = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (iterator) {
        const esp_partition_t *part = esp_partition_get(iterator);
        char buf[100];
        snprintf(buf, sizeof(buf),"%d %x %x %x %s\n", part->type, part->subtype, part->address, part->size, part->label);
        httpd_resp_send_chunk(req, buf, strlen(buf));
        iterator = esp_partition_next(iterator);
    }
    esp_partition_iterator_release(iterator);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t cmd_post_handler(httpd_req_t *req)
{
    char buf[100] = {0};
    int ret, remaining = req->content_len;

    ESP_LOGI(TAG, "Received %d bytes on %s", req->content_len, req->uri);
    /* Read the data for the request */
    if ((ret = httpd_req_recv(req, buf,
                    MIN(remaining, sizeof(buf)))) <= 0) {
        return app_httpd_report_error(req, "failure", "httpd read failed");
    }
    ESP_LOGI(TAG, "Got data %s %d", buf, ret);
    jparse_ctx_t jctx;
    if (json_parse_start(&jctx, buf, ret) != 0) {
        return app_httpd_report_error(req, "failure", "JSON parse failed");
    }
    char cmd[10] = {0};
    char val[128] = {0};
    size_t cmd_size = sizeof(cmd);
    size_t val_size = sizeof(val);
    esp_err_t err = ESP_FAIL;
    if ((json_obj_get_string(&jctx, "cmd", cmd, cmd_size) == 0 ) &&
            (json_obj_get_string(&jctx, "val", val, val_size) == 0 )) {
        if (strcmp(cmd, "ssid") == 0) {
            ESP_LOGI(TAG, "Set SSID: %s", val);
            err = mdf_info_save("ssid", val, strlen(val) + 1);
        } else if (strcmp(cmd, "reboot") == 0) {
            app_httpd_report_error(req, "success", "Rebooting...");
            app_reboot_timer_start(5000);
        } else if (strcmp(cmd, "partition") == 0) {
            app_partition_iterate(req);
        } else {
            json_parse_end(&jctx);
            return app_httpd_report_error(req, "failure", "Invalid command");
        }
    } else {
        json_parse_end(&jctx);
        return app_httpd_report_error(req, "failure", "JSON get failed");
    }
    json_parse_end(&jctx);
    if (err == ESP_OK) {
        app_httpd_report_error(req, "success", "Command executed successfully");
    } else {
        app_httpd_report_error(req, "failure", "Command execution failed");
    }
    return ESP_OK;
}

httpd_uri_t command_handler = {
    .uri       = "/command",
    .method    = HTTP_POST,
    .handler   = cmd_post_handler,
    .user_ctx  = NULL
};


static httpd_handle_t server = NULL;
httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 12;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ota_handler);
        //httpd_register_uri_handler(server, &light_set_handler);
        //httpd_register_uri_handler(server, &light_get_handler);
        httpd_register_uri_handler(server, &light_ctrl_handler);
        httpd_register_uri_handler(server, &command_handler);
    } else {
        ESP_LOGE(TAG, "Error starting server!");
        return NULL;
    }

    return server;
}
static void ota_timer_cb(void *arg)
{
    ESP_LOGI(TAG, "Rebooting Now -----");
    esp_restart();
}

esp_err_t app_reboot_timer_create(void)
{
    esp_timer_init();
    esp_timer_create_args_t timer_conf = {
        .callback = ota_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ota_tm"
    };
    esp_err_t ret = esp_timer_create(&timer_conf, &ota_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer");
    }
    return ret;
}

void app_reset_to_factory(void)
{
    nvs_flash_erase();
    esp_restart();
}

