#include "wifi_manager.h"
#include "esp_log.h"

// ============================================================
// SWITCH DE ENTORNO: 0 para ESP32 físico, 1 para QEMU
#define QEMU_SIMULATION_MODE 0
// ============================================================

static const char *TAG = "wifi_manager";
static int connected = 0;

#if !QEMU_SIMULATION_MODE
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAX_RETRIES        5

static EventGroupHandle_t wifi_event_group;
static int retry_count = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconn = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGE(TAG, "El router rechazó la conexion. Codigo de error: %d", disconn->reason);
        connected = 0;
        if (retry_count < MAX_RETRIES) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGW(TAG, "Reintentando conexion al Wi-Fi (%d/%d)...", retry_count, MAX_RETRIES);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        connected = 1;
        retry_count = 0; // Reseteamos el contador al conectar exitosamente
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
#endif

int wifi_connect(const char *ssid, const char *password) {
#if QEMU_SIMULATION_MODE
    ESP_LOGI(TAG, "Simulando conexion a red: %s", ssid);
    connected = 1;
    ESP_LOGI(TAG, "Conexion simulada exitosa");
    return 0;
#else
    // 1. Inicializar la memoria Flash no volátil (Obligatorio para Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    
    // 2. Configurar el stack TCP/IP y el Wi-Fi
    wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    // 3. Registrar los callbacks para manejar la conexión
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &event_handler, NULL, &instance_got_ip);
    
    // 4. Configurar credenciales y arrancar
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_set_max_tx_power(40);
    ESP_LOGI(TAG, "Conectando a la red fisica: %s", ssid);
    
    // 5. Esperar hasta que se conecte o falle (Timeout de 10 seg)
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(10000));
                                           
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Conectado al Wi-Fi exitosamente");
        return 0;
    }
    
    ESP_LOGE(TAG, "Fallo al conectar al Wi-Fi");
    return -1;
#endif
}

int wifi_is_connected(void) {
    return connected;
}

void wifi_disconnect(void) {
#if QEMU_SIMULATION_MODE
    connected = 0;
    ESP_LOGI(TAG, "Desconectado (Simulado)");
#else
    esp_wifi_disconnect();
    esp_wifi_stop();
    connected = 0;
    ESP_LOGI(TAG, "Desconectado del Wi-Fi real");
#endif
}