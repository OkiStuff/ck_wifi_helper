/*

    CK Wifi Helper 1.0.0
    Copyright (c) OkiStuff 2022

    This project is licensed under the MIT License. This notice must also be included with all copies of this library.

    CK Wifi Helper is a high-level wifi interface for the ESP32 Platform.
    ```c
    #include <stdio.h>
    #include "nvs_flash".h
    #define CK_WIFI_HELPER_IMPLEMENTION
    #include "ck_wifi_helper/wifi.h"

    static const char* TAG = "CK Wifi Helper Example";

    void app_main(void)
    {
        ck_event_data_init();
        esp_err_t wifi_status = CK_WIFI_FAILURE;

        esp_err_t nvs_flash_status = nvs_flash_init();
        if (nvs_flash_status == ESP_ERR_NVS_NO_FREE_PAGES || nvs_flash_status == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            nvs_flash_status = nvs_flash_init();
        }

        ESP_ERROR_CHECK(nvs_flash_status);

        wifi_status = ck_connect_wifi("SSID", "Password", CK_WIFI_AUTH_WPA2_PSK);
        if (wifi_status != CK_WIFI_SUCCESS)
        {
            ESP_LOGI(TAG, "Failed to connect to access point, exiting...");
            return;
        }
    }
    ```

    Big thanks to Low Level Learning on youtube!
    https://www.youtube.com/c/LowLevelLearning
*/

#ifndef CK_WIFI_HELPER_H
#define CK_WIFI_HELPER_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#define CK_WIFI_SUCCESS 1 << 0
#define CK_WIFI_FAILURE 1 << 1

#ifndef CK_MAX_FAILURES
    #define CK_MAX_FAILURES 10
#endif

typedef struct ck_wifi_helper_data_t
{
    EventGroupHandle_t wifi_event_group;
    int retry_num;
} ck_wifi_helper_data_t;

typedef enum
{
    CK_WIFI_AUTH_OPEN = 0,
    CK_WIFI_AUTH_WEP,
    CK_WIFI_AUTH_WPA_PSK,
    CK_WIFI_AUTH_WPA2_PSK,
    CK_WIFI_AUTH_WPA_WPA2_PSK,
    CK_WIFI_AUTH_WPA2_ENTERPRISE,
    CK_WIFI_AUTH_WPA3_PSK,
    CK_WIFI_AUTH_WPA2_WPA3_PSK,
    CK_WIFI_AUTH_WAPI_PSK,
    CK_WIFI_AUTH_OWE,
    CK_WIFI_AUTH_MAX
} ck_wifi_auth_mode_t;

static ck_wifi_helper_data_t ck_wifi_event_data;
static const char* CK_WIFI_HELPER_TAG_NAME = "ck_wifi_helper";

static inline void ck_event_data_init();

static void ck_callback_wifi_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void ck_callback_ip_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

esp_err_t ck_connect_wifi(const char* ssid, const char* password, ck_wifi_auth_mode_t wifi_auth_mode);

#ifdef CK_WIFI_HELPER_IMPLEMENTION

static inline void ck_event_data_init()
{
    ck_wifi_event_data.retry_num = 0;
}

static void ck_callback_wifi_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(CK_WIFI_HELPER_TAG_NAME, "Connecting to access point...");
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (ck_wifi_event_data.retry_num < CK_MAX_FAILURES)
        {
            ESP_LOGI(CK_WIFI_HELPER_TAG_NAME, "Reconnecting to access point...");
            esp_wifi_connect();

            ck_wifi_event_data.retry_num++;
        }

        else xEventGroupSetBits(ck_wifi_event_data.wifi_event_group, CK_WIFI_FAILURE);
    }
}

static void ck_callback_ip_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)(event_data);
        ESP_LOGI(CK_WIFI_HELPER_TAG_NAME, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));

        ck_wifi_event_data.retry_num = 0;
        xEventGroupSetBits(ck_wifi_event_data.wifi_event_group, CK_WIFI_SUCCESS);
    }
}

esp_err_t ck_connect_wifi(const char* ssid, const char* password, ck_wifi_auth_mode_t wifi_auth_mode)
{
    int status = CK_WIFI_FAILURE;

    // initialize wifi interface and station //

    ESP_ERROR_CHECK(esp_netif_init()); // esp network interface
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // esp event loop
    esp_netif_create_default_wifi_sta(); // wifi station

    //  Initialize Event Loop //

    ck_wifi_event_data.wifi_event_group = xEventGroupCreate();

    esp_event_handler_instance_t wifi_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ck_callback_wifi_event, NULL, &wifi_handler_event_instance));

    esp_event_handler_instance_t got_ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ck_callback_ip_event, NULL, &got_ip_event_instance));

    // Start the Wifi //

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = *ssid,
            .password = *password,
            .threshold.authmode = (wifi_auth_mode_t)(wifi_auth_mode),

            .pmf_cfg = {
                .capable = 1,
                .required = 0
            }
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // set wifi controller to be station
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // set wifi config
    ESP_ERROR_CHECK(esp_wifi_start()); // start wifi

    ESP_LOGI(CK_WIFI_HELPER_TAG_NAME, "STA initialization complete!");

    EventBits_t bits = xEventGroupWaitBits(ck_wifi_event_data.wifi_event_group, CK_WIFI_SUCCESS | CK_WIFI_FAILURE, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & CK_WIFI_SUCCESS)
    {
        ESP_LOGI(CK_WIFI_HELPER_TAG_NAME, "Connected to access point!");
        status = CK_WIFI_SUCCESS;
    }

    else if (bits & CK_WIFI_FAILURE)
    {
        ESP_LOGI(CK_WIFI_HELPER_TAG_NAME, "Failed to conect to access point...");
        status = CK_WIFI_FAILURE;
    }

    else
    {
        ESP_LOGE(CK_WIFI_HELPER_TAG_NAME, "Unexpected event...");
        status = CK_WIFI_FAILURE;
    }

    // Unregister event loop //

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_event_instance));
    vEventGroupDelete(ck_wifi_event_data.wifi_event_group);

    return status;
}
#endif // CK_WIFI_HELPER_IMPLEMENTION
#endif // CK_WIFI_HELPER_H