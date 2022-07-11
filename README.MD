# CK Wifi Helper
CK Wifi Helper is a small, lightweight, single-header, high-level abstraction of the WiFi interface for the ESP32 Platform. Quickly get your ESP32 Microcontroller connected to a WiFi access point by calling a few functions.

## Getting Started
First we need to clone the repo. Make sure you have git installed.

```
$ git clone https://github.com/okistuff/ck_wifi_helper
```

### ESP-IDF Global Install
If you use the ESP-IDF framework, you can quickly install this library globally by running the install script under `./install_scripts/`.

```
$ py idf_install_ck.py
idf-install.py tool 0.1.0
Where is your ESP-IDF Installation? /path/to/esp/idf
-----------------------------------------------------

-chdir /path/to/esp/idf
-mkdir ck_wifi_helper
-chdir ck_wifi_helper
...
```

> You can also just copy the ck_wifi_helper folder to `/esp-idf/components/`


### Local Install
If you want to install this library to a specific project, you can either move the include folder to that project or just install the library with `git` in the project folder. The repository is small so it will not take up storage or take a while to install. If you have a `git` repository initialized in your project, you can also add it as a submodule (this is the recommended way to use this library locally). If you do take the route of just installing it locally and not as a submodule, make sure the license is findable somewhere and all notices is kept as found.

### Using the library
> This example was made for FreeRTOS and with the ESP-IDF framework. Should still work with other configurations, but may need some tweaking

First, let's create a `wifi_config.h` header
```h
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#define CK_ENV_WIFI_SSID "your-ssid"
#define CK_ENV_WIFI_PASSWORD "your-password"

#endif // WIFI_CONFIG_H
```
> This can kind of work as like a `.env` file, just make sure to `.gitignore` it/just do not share this file.

Now, we can start using ck_wifi_helper in the main file
```c
#include <stdio.h>
#include "wifi_config.h"
#define CK_WIFI_HELPER_IMPLEMENTION
#include "ck_wifi_helper/wifi.h"
#include "nvs_flash.h"

static const char* TAG = "main";

void app_main(void)
{
    ck_event_data_init(); // initialize event data
    esp_err_t wifi_status = CK_WIFI_FAILURE; // set default value to failure

    esp_err_t nvs_flash_status = nvs_flash_init();
    if (nvs_flash_status == ESP_ERR_NVS_NO_FREE_PAGES || nvs_flash_status == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_status = nvs_flash_init();
    }

    ESP_ERROR_CHECK(nvs_flash_status);

    wifi_status = ck_connect_wifi(CK_WIFI_AUTH_WPA2_PSK);
    if (wifi_status != CK_WIFI_SUCCESS)
    {
        ESP_LOGI(TAG, "Failed to connect to access point, exiting...");
        return;
    }
}
```

This will just connect to access point and then wait for it to quit. Now, let's change the file to connect to a TCP server

```c
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_config.h"
#define CK_WIFI_HELPER_IMPLEMENTION
#include "ck_wifi_helper/wifi.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define TCP_SUCCESS 1 << 0
#define TCP_FAILURE 1 << 1

#define TCP_TARGET_HOST "127.0.0.1"
#define TCP_TARGET_PORT 12345

static const char* TAG = "main";

esp_err_t connect_tcp_server(void)
{
    struct sockaddr_in server_info = {0};
    char buffer[1024] = {0}; // uses 1k of memory

    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(TCP_TARGET_HOST);
    server_info.sin_port = htons(TCP_TARGET_PORT);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        ESP_LOGE(TASK_TAG, "Failed to create socket... ?????????????????");
        return TCP_FAILURE;
    }

    if (connect(sock, (struct sockaddr*)(&server_info), sizeof(server_info)) != 0)
    {
        ESP_LOGE(TASK_TAG, "Failed to connect to %s... :(", inet_ntoa(server_info.sin_addr.s_addr));
        close(sock);

        return TCP_FAILURE;
    }

    ESP_LOGI(TASK_TAG, "Connected to TCP Server!");
    bzero(buffer, sizeof(buffer));

    int r = read(sock, buffer, sizeof(buffer) - 1);
    for (int i = 0; i < r; i++)
    {
        putchar(buffer[i]);
    }

    if (strcmp(buffer, "HELLO") == 0)
    {
        ESP_LOGI(TASK_TAG, "Got 'HELLO' ping");
    }

    return TCP_SUCCESS;
}

void app_main(void)
{
    ck_event_data_init(); // initialize event data
    esp_err_t wifi_status = CK_WIFI_FAILURE; // set default value to failure

    esp_err_t nvs_flash_status = nvs_flash_init();
    if (nvs_flash_status == ESP_ERR_NVS_NO_FREE_PAGES || nvs_flash_status == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_status = nvs_flash_init();
    }

    ESP_ERROR_CHECK(nvs_flash_status);

    wifi_status = ck_connect_wifi(CK_WIFI_AUTH_WPA2_PSK);
    if (wifi_status != CK_WIFI_SUCCESS)
    {
        ESP_LOGI(TAG, "Failed to connect to access point, exiting...");
        return;
    }
}
```

We can spin up a TCP server on our PC with `nc -lvvp 12345`.

## Documentation

`#define CK_WIFI_SUCCESS 1 << 0` <br />
Macro representing a successful connection to a WiFi access point

`#define CK_WIFI_FAILURE 1 << 1` <br />
Macro representing a non-successful/failed connection to a WiFi access point

`#define CK_MAX_FAILURES 10` <br />
Macro representing the amount of non-sccessful connections to a WiFi access point until failure. Can be changed by defining macro before inclusion of library.

`#define CK_ENV_WIFI_SSID` <br />
Undefined macro representing WiFi access point SSID. Set by user

`#define CK_ENV_WIFI_PASSWORD` <br />
Undefined macro representing WiFi access point password. Set by user

`typedef struct ck_wifi_helper_data_t ck_wifi_helper_data_t` <br />
Internal structure with important data.

```c
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
```
C enum with values representing different WiFi auth modes. 1-to-1 match with `wifi_auth_mode_t`

`static inline void ck_event_data_init()` <br />
Initialize local variables for ck_wifi_helper

`static void ck_callback_wifi_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)` <br />
Internal callback function for WiFi connect event

`static void ck_callback_ip_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)` <br />
Internal callback functon for IP got event

`esp_err_t ck_connect_wifi(ck_wifi_auth_mode_t wifi_auth_mode)` <br />
Abstraction function to connect to WiFi access point.