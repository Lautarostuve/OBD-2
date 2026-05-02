#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pid_parser.h"
#include "json_builder.h"
#include "wifi_manager.h"
#include "http_sender.h"

#define WIFI_SSID     "kankel"
#define WIFI_PASSWORD "indio22280"
#define BACKEND_URL   "https://webhook.site/0ad20dae-8432-4d7a-9a57-112d02a5b900"

void app_main(void) {

    // Conectar al Wi-Fi
    ESP_LOGI("main", "Conectando al Wi-Fi...");
    if (wifi_connect(WIFI_SSID, WIFI_PASSWORD) != 0) {
        ESP_LOGE("main", "No se pudo conectar al Wi-Fi");
        return;
    }

    while (1) {
        // Leer datos simulados del motor
        EngineData data = {0};
        parse_pid(0x0C, 0x1F, 0x40, &data);
        parse_pid(0x0D, 0x5A, 0x00, &data);
        parse_pid(0x05, 0x7D, 0x00, &data);
        parse_pid(0x2F, 0x72, 0x00, &data);

        // Generar JSON
        char json_output[JSON_BUFFER_SIZE];
        int result = build_json(&data, "ABC-123", 1713374200,
                                json_output, JSON_BUFFER_SIZE);

        if (result == 0) {
            ESP_LOGI("main", "JSON: %s", json_output);

            // Enviar al backend si hay conexión
            if (wifi_is_connected()) {
                http_post_json(BACKEND_URL, json_output);
            }
        }

        // Esperar 5 segundos antes de la próxima lectura
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}