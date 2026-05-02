#include "wifi_manager.h"
#include "esp_log.h"

static const char *TAG = "wifi_manager";
static int connected = 0;

// ============================================================
// MODO SIMULACION: descomentar cuando haya hardware real
// ============================================================
// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "nvs_flash.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/event_groups.h"
// #include <string.h>
// 
// #define WIFI_CONNECTED_BIT BIT0
// #define WIFI_FAIL_BIT      BIT1
// #define MAX_RETRIES        5
// 
// static EventGroupHandle_t wifi_event_group;
// static int retry_count = 0;
// 
// static void event_handler(void *arg, esp_event_base_t event_base,
//                           int32_t event_id, void *event_data) {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//         esp_wifi_connect();
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         connected = 0;
//         if (retry_count < MAX_RETRIES) {
//             esp_wifi_connect();
//             retry_count++;
//         } else {
//             xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
//         }
//     } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
//         connected = 1;
//         xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
//     }
// }
// 
// int wifi_connect(const char *ssid, const char *password) {
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//         nvs_flash_erase();
//         nvs_flash_init();
//     }
//     wifi_event_group = xEventGroupCreate();
//     esp_netif_init();
//     esp_event_loop_create_default();
//     esp_netif_create_default_wifi_sta();
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     esp_wifi_init(&cfg);
//     esp_event_handler_instance_t instance_any_id;
//     esp_event_handler_instance_t instance_got_ip;
//     esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
//                                         &event_handler, NULL, &instance_any_id);
//     esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
//                                         &event_handler, NULL, &instance_got_ip);
//     wifi_config_t wifi_config = {0};
//     strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
//     strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
//     esp_wifi_set_mode(WIFI_MODE_STA);
//     esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
//     esp_wifi_start();
//     EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
//                                            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//                                            pdFALSE, pdFALSE,
//                                            pdMS_TO_TICKS(10000));
//     if (bits & WIFI_CONNECTED_BIT) return 0;
//     return -1;
// }
// ============================================================

int wifi_connect(const char *ssid, const char *password) {
    ESP_LOGI(TAG, "Simulando conexion a red: %s", ssid);
    connected = 1;
    ESP_LOGI(TAG, "Conexion simulada exitosa");
    return 0;
}

int wifi_is_connected(void) {
    return connected;
}

void wifi_disconnect(void) {
    connected = 0;
    ESP_LOGI(TAG, "Desconectado");
}