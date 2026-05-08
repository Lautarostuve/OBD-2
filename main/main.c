#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pid_parser.h"
#include "json_builder.h"
#include "wifi_manager.h"
#include "http_sender.h"
#include "data_buffer.h"
#include "time_manager.h"
#include "can_driver.h"

#define WIFI_SSID     "kankel-2.4Ghz"
#define WIFI_PASSWORD "indio22280"
#define BACKEND_URL   "https://webhook.site/0ad20dae-8432-4d7a-9a57-112d02a5b900"
#define VEHICLE_ID    "ABC-123"
#define MAX_HTTP_RETRIES 3


#define SIMULATE_NO_WIFI 0 // Cambia a 1 para probar el buffer sin Wi-Fi

static const char *TAG = "main";

void app_main(void) {

    buffer_init();

    // Conectar al Wi-Fi
    ESP_LOGI(TAG, "Conectando al Wi-Fi...");
    if (wifi_connect(WIFI_SSID, WIFI_PASSWORD) != 0) {
        ESP_LOGE(TAG, "No se pudo conectar al Wi-Fi");
        return;
    }

    // Sincronizar hora con NTP
    ESP_LOGI(TAG, "Sincronizando hora...");
    if (time_sync() != 0) {
        ESP_LOGW(TAG, "Sin hora NTP, usando timestamp 0");
    }

    while (1) {
        // Leer datos del motor (simulados por ahora)
        EngineData data = {0};
        
        // PIDs que queremos leer
        uint8_t pids_to_read[] = {0x0C, 0x0D, 0x05, 0x2F};
        
        for (int i = 0; i < 4; i++) {
            uint8_t current_pid = pids_to_read[i];
            
            // 1. Pedimos el PID al motor
            if (can_request_pid(current_pid) == 0) {
                
                uint8_t r_pid, r_A, r_B;
                // 2. Esperamos y leemos la respuesta
                if (can_receive_data(&r_pid, &r_A, &r_B) == 0) {
                    
                    // 3. Pasamos los bytes crudos a tu parser matemático
                    if (r_pid == current_pid) {
                        parse_pid(r_pid, r_A, r_B, &data);
                    }
                }
            }
            // Pequeña pausa entre preguntas para no saturar la computadora del auto (muy importante en la realidad)
            vTaskDelay(pdMS_TO_TICKS(50)); 
        }

        // Obtener timestamp real
        long timestamp = time_get_timestamp();

        // Guardar lectura en el buffer
        buffer_push(&data, VEHICLE_ID, timestamp);

        // Solo intenta enviar si hay red Y hay al menos una lectura en el buffer
        if (wifi_is_connected() && !buffer_is_empty() && !SIMULATE_NO_WIFI) {
            int items_to_send = buffer_count();
            ESP_LOGI(TAG, "Iniciando ráfaga: intentando enviar %d lecturas...", items_to_send);

            char json_output[JSON_BUFFER_SIZE];
            int successfully_sent = 0;

            for (int i = 0; i < items_to_send; i++) {
                const BufferedReading *reading = buffer_get(i);

                if (build_json(&reading->data, reading->vehicle_id, 
                            reading->timestamp, json_output, JSON_BUFFER_SIZE) == 0) {
                    
                    int attempt = 0;
                    int sent_ok = 0;

                    // Bucle de reintentos para CADA lectura
                    while (attempt < MAX_HTTP_RETRIES && !sent_ok) {
                        if (http_post_json(BACKEND_URL, json_output) == 0) {
                            sent_ok = 1; 
                        } else {
                            attempt++;
                            ESP_LOGW(TAG, "Error en POST. Intento %d/%d fallido...", attempt, MAX_HTTP_RETRIES);
                            if (attempt < MAX_HTTP_RETRIES) {
                                vTaskDelay(pdMS_TO_TICKS(1000)); // Pausa de 1 seg antes de reintentar
                            }
                        }
                    }

                    if (sent_ok) {
                        successfully_sent++;
                    } else {
                        // Si falló 3 veces seguidas, asumimos que se cayó el Wi-Fi o el Backend.
                        ESP_LOGE(TAG, "Fallo definitivo tras %d intentos. Abortando ráfaga para no perder datos.", MAX_HTTP_RETRIES);
                        break; // Cortamos el 'for'. Los datos restantes quedan seguros en el array.
                    }
                }
            }

            // Actualizamos el buffer borrando SOLO los que llegaron al servidor
            if (successfully_sent > 0) {
                buffer_remove_first_n(successfully_sent);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}