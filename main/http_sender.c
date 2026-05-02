#include "http_sender.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "http_sender";

// ============================================================
// MODO SIMULACION: descomentar cuando haya servidor real
// ============================================================
// #include "esp_http_client.h"
// 
// static int http_status_code = 0;
// 
// static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
//     if (evt->event_id == HTTP_EVENT_ON_FINISH) {
//         http_status_code = esp_http_client_get_status_code(evt->client);
//         ESP_LOGI(TAG, "Respuesta del servidor: %d", http_status_code);
//     }
//     return ESP_OK;
// }
// 
// int http_post_json(const char *url, const char *json_body) {
//     esp_http_client_config_t config = {
//         .url = url,
//         .event_handler = http_event_handler,
//         .method = HTTP_METHOD_POST,
//     };
//     esp_http_client_handle_t client = esp_http_client_init(&config);
//     esp_http_client_set_header(client, "Content-Type", "application/json");
//     esp_http_client_set_post_field(client, json_body, strlen(json_body));
//     esp_err_t err = esp_http_client_perform(client);
//     esp_http_client_cleanup(client);
//     if (err == ESP_OK && http_status_code == 200) {
//         ESP_LOGI(TAG, "POST exitoso");
//         return 0;
//     }
//     ESP_LOGE(TAG, "Error en POST: %s", esp_err_to_name(err));
//     return -1;
// }
// ============================================================

int http_post_json(const char *url, const char *json_body) {
    ESP_LOGI(TAG, "POST simulado a: %s", url);
    ESP_LOGI(TAG, "Body: %s", json_body);
    ESP_LOGI(TAG, "Respuesta simulada: 200 OK");
    return 0;
}