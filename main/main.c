#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
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

// Configuración de la máquina de estados
#define PARKED_THRESHOLD_CYCLES 6 // 6 ciclos de 5 segundos = 30 segundos de motor apagado

static const char *TAG = "main";

void app_main(void) {

    buffer_init();

    // Ya no intentamos conectar al Wi-Fi ni buscar la hora al arrancar.
    // Asumimos que el auto acaba de encender y estamos en la calle.
    ESP_LOGI(TAG, "=== SISTEMA AUTOLOG INICIADO ===");

    // ==========================================
    // CHEQUEO RÁPIDO POST-DESPERTAR
    // ==========================================
    // esp_sleep_get_wakeup_cause() nos dice si arrancó por primera vez o si lo despertó el timer
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
        
        // --- TRUCO DE DEBUG ---
        // Frenamos el procesador 3 segundos ANTES de hacer nada.
        // Esto le da tiempo a Windows de reconectar el USB y abrir el monitor serie. Es solo para debugear, en produccion sacar.
        vTaskDelay(pdMS_TO_TICKS(3000)); 
        
        ESP_LOGI(TAG, "¡HOLA WINDOWS! Desperté por timer. Verificando si el motor está vivo...");
        
        if (can_request_pid(0x0C) != 0) { 
            ESP_LOGW(TAG, "Silencio en el bus CAN. El auto sigue apagado. Volviendo a dormir 3 min.");
            
            // Pausa extra para que llegues a leer el mensaje en rojo/amarillo antes de que muera
            vTaskDelay(pdMS_TO_TICKS(500)); 
            
            esp_sleep_enable_timer_wakeup(180 * 1000000ULL); 
            esp_deep_sleep_start(); 
        } else {
            ESP_LOGI(TAG, "¡El motor respondió! Iniciando Modo Vuelo para grabar el viaje.");
        }
    } else {
        ESP_LOGI(TAG, "Arranque inicial (Power On / Reset manual).");
    }

    ESP_LOGI(TAG, "Modo Vuelo Activo. Wi-Fi apagado. Iniciando recolección...");
    int motor_apagado_count = 0;

    while (1) {
        
        EngineData data = {0}; 
        uint8_t pids_to_read[] = {0x0C, 0x0D, 0x05, 0x2F}; // 0x0C es RPM
        
        /* ==========================================
         * FASE 1: RECOLECCIÓN (Modo Vuelo)
         * ========================================== */
        for (int i = 0; i < 4; i++) {
            uint8_t current_pid = pids_to_read[i];
            
            if (can_request_pid(current_pid) == 0) {
                uint8_t r_pid, r_A, r_B;
                if (can_receive_data(&r_pid, &r_A, &r_B) == 0) {
                    if (r_pid == current_pid) {
                        parse_pid(r_pid, r_A, r_B, &data);
                    }
                }
            }
            vTaskDelay(pdMS_TO_TICKS(50)); 
        }

        // Si no tenemos Wi-Fi, el timestamp será el tiempo desde que arrancó el chip.
        // Es un timestamp relativo, pero sirve para ordenar los eventos.
        long timestamp = time_get_timestamp();
        buffer_push(&data, VEHICLE_ID, timestamp);

        /* ==========================================
         * FASE 2: DETECCIÓN DE GARAGE (Máquina de Estados)
         * ========================================== */
         
        // Asumimos que si las RPM son exactamente 0.0, el motor está apagado.
        if (data.rpm == 0.0) {
            motor_apagado_count++;
            ESP_LOGI(TAG, "Motor apagado detectado. Conteo para sincronizar: %d/%d", 
                     motor_apagado_count, PARKED_THRESHOLD_CYCLES);
        } else {
            // Si el motor gira, reiniciamos el contador. Seguimos en Modo Vuelo.
            motor_apagado_count = 0; 
        }

        /* ==========================================
         * FASE 3: MODO SINCRONIZACIÓN Y SUEÑO PROFUNDO
         * ========================================== */
        if (motor_apagado_count >= PARKED_THRESHOLD_CYCLES) {
            
            ESP_LOGI(TAG, "Vehículo estacionado confirmado. Iniciando Modo Sincronización...");
            
            int max_intentos_generales = 10; // Intentará vaciar el buffer hasta 10 veces antes de rendirse (Esto se puede aumentar a mas) (10 es muy poco)
            int intento_actual = 0;

            // BUCLE DE INSISTENCIA: Se queda acá adentro hasta vaciar la memoria o agotar intentos
            while (!buffer_is_empty() && intento_actual < max_intentos_generales) {
                
                // Si no hay red, intentamos conectar o reconectar
                if (!wifi_is_connected()) {
                    ESP_LOGI(TAG, "Buscando red del garage...");
                    wifi_connect(WIFI_SSID, WIFI_PASSWORD);
                }

                if (wifi_is_connected()) {
                    // time_sync(); // (Opcional)
                    
                    int items_to_send = buffer_count();
                    ESP_LOGI(TAG, "Descargando buffer: %d lecturas pendientes (Intento general %d/%d)", 
                             items_to_send, intento_actual + 1, max_intentos_generales);

                    char json_output[JSON_BUFFER_SIZE];
                    int successfully_sent = 0;

                    for (int i = 0; i < items_to_send; i++) {
                        const BufferedReading *reading = buffer_get(i);

                        if (build_json(&reading->data, reading->vehicle_id, reading->timestamp, json_output, JSON_BUFFER_SIZE) == 0) {
                            
                            int attempt = 0;
                            int sent_ok = 0;

                            while (attempt < MAX_HTTP_RETRIES && !sent_ok) {
                                if (http_post_json(BACKEND_URL, json_output) == 0) {
                                    sent_ok = 1; 
                                } else {
                                    attempt++;
                                    vTaskDelay(pdMS_TO_TICKS(1500)); // Pausa si falla
                                }
                            }

                            if (sent_ok) {
                                successfully_sent++;
                                // Esta pausa luego hay que sacarla, no tiene sentido en contexto de produccion con un server real.
                                vTaskDelay(pdMS_TO_TICKS(1000)); 
                            } else {
                                ESP_LOGW(TAG, "El servidor rechazó este paquete. Frenando ráfaga actual.");
                                break; // Rompe el FOR, pero NO el WHILE. 
                            }
                        }
                    }

                    if (successfully_sent > 0) {
                        buffer_remove_first_n(successfully_sent);
                        ESP_LOGI(TAG, "Se confirmaron %d registros guardados en la nube.", successfully_sent);
                    }

                    if (buffer_is_empty()) {
                        ESP_LOGI(TAG, "¡Sincronización perfecta! La memoria está limpia.");
                        break; // Memoria vacía, podemos salir del bucle y dormir felices.
                    } else {
                        ESP_LOGW(TAG, "Aún quedan datos en memoria. Reintentando ráfaga en 5 segundos...");
                        vTaskDelay(pdMS_TO_TICKS(5000)); // Pausa antes de intentar mandar el resto
                    }

                } else {
                    ESP_LOGW(TAG, "No hay Wi-Fi disponible. Reintentando en 10 segundos...");
                    vTaskDelay(pdMS_TO_TICKS(10000));
                }
                
                intento_actual++;
            }

            // 4. A DORMIR PARA NO GASTAR BATERÍA DEL AUTO
            if (!buffer_is_empty()) {
                ESP_LOGE(TAG, "Se agotaron los %d intentos y no se pudo vaciar la memoria. Protegiendo batería...", max_intentos_generales);
            }
            
            ESP_LOGI(TAG, "Configurando alarma para despertar en 3 minutos...");
            // La función recibe MICROsegundos.
            // 180 segundos * 1.000.000. La 'ULL' es para decirle a C que es un número gigante (Unsigned Long Long)
            esp_sleep_enable_timer_wakeup(180 * 1000000ULL);

            ESP_LOGI(TAG, "Apagando sistemas. Entrando en Deep Sleep...");
            if (wifi_is_connected()) {
                wifi_disconnect(); 
            }
            vTaskDelay(pdMS_TO_TICKS(1000)); 
            esp_deep_sleep_start(); 
        }

        // Frecuencia de muestreo mientras manejamos: 5 segundos.
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}