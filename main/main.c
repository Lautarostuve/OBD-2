#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pid_parser.h"
#include "json_builder.h"
#include "wifi_manager.h"
#include "http_sender.h"
#include "data_buffer.h"

#define WIFI_SSID     "kankel"
#define WIFI_PASSWORD "tu_contraseña"
#define BACKEND_URL   "https://webhook.site/0ad20dae-8432-4d7a-9a57-112d02a5b900"
#define VEHICLE_ID    "ABC-123"

static const char *TAG = "main";

void app_main(void) {

    // Conectar al Wi-Fi
    ESP_LOGI(TAG, "Conectando al Wi-Fi...");
    if (wifi_connect(WIFI_SSID, WIFI_PASSWORD) != 0) {
        ESP_LOGE(TAG, "No se pudo conectar al Wi-Fi");
        return;
    }

    long timestamp = 1713374200;

    while (1) {
        // Leer datos del motor (simulados por ahora)
        EngineData data = {0};
        parse_pid(0x0C, 0x1F, 0x40, &data);
        parse_pid(0x0D, 0x5A, 0x00, &data);
        parse_pid(0x05, 0x7D, 0x00, &data);
        parse_pid(0x2F, 0x72, 0x00, &data);

        // Guardar lectura en el buffer
        buffer_push(&data, VEHICLE_ID, timestamp);
        timestamp += 5; // simula el paso del tiempo

        // Mandar cuando el buffer este lleno o haya conexion
        if (buffer_is_full() || wifi_is_connected()) {
            ESP_LOGI(TAG, "Enviando %d lecturas al backend...", buffer_count());

            char json_output[JSON_BUFFER_SIZE];

            for (int i = 0; i < buffer_count(); i++) {
                const BufferedReading *reading = buffer_get(i);

                int result = build_json(&reading->data, reading->vehicle_id,
                                        reading->timestamp, json_output, JSON_BUFFER_SIZE);

                if (result == 0) {
                    http_post_json(BACKEND_URL, json_output);
                }
            }

            // Limpiar buffer después de mandar
            buffer_clear();
        }

        // Esperar 5 segundos antes de la próxima lectura
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}