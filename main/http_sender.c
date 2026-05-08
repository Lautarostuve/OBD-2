#include "http_sender.h"
#include "esp_log.h"
#include <string.h>

// ============================================================
// SWITCH DE ENTORNO: 0 para ESP32 físico, 1 para QEMU
#define QEMU_SIMULATION_MODE 0
// ============================================================

static const char *TAG = "http_sender";

#if !QEMU_SIMULATION_MODE
#include "esp_http_client.h"
#include "esp_crt_bundle.h" // Vital para que funcione con URLs "https://"

static int http_status_code = 0;

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_FINISH) {
        http_status_code = esp_http_client_get_status_code(evt->client);
    }
    return ESP_OK;
}
#endif

int http_post_json(const char *url, const char *json_body) {
#if QEMU_SIMULATION_MODE
    ESP_LOGI(TAG, "POST simulado a: %s", url);
    ESP_LOGI(TAG, "Body: %s", json_body);
    ESP_LOGI(TAG, "Respuesta simulada: 200 OK");
    return 0;
#else
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach, // Adjunta los certificados raíz
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // Configuramos los headers y el body del POST
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_body, strlen(json_body));
    
    ESP_LOGI(TAG, "Enviando POST real a la nube...");
    ESP_LOGI(TAG, "Payload: %s", json_body);
    
    // Disparamos la petición
    esp_err_t err = esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    
    if (err == ESP_OK) {
        if (http_status_code == 200) {
            ESP_LOGI(TAG, "¡POST exitoso! Respuesta del servidor: 200 OK");
            return 0;
        } else {
            ESP_LOGE(TAG, "Servidor rechazó el POST. Status HTTP: %d", http_status_code);
            return -1; // ← esto hace que main.c NO borre el buffer
        }
    }
    
    ESP_LOGE(TAG, "Falló el POST. Error de ESP-IDF: %s | Status HTTP: %d", esp_err_to_name(err), http_status_code);
    return -1;
#endif
}