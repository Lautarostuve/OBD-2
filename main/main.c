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
#include "web_server.h"
#include "nvs_flash.h"

#define WIFI_SSID     "kankel-2.4Ghz"
#define WIFI_PASSWORD "indio22280"
#define BACKEND_URL   "https://webhook.site/df87d0ec-e564-461d-8a13-759e4f49e74b"
#define VEHICLE_ID    "ABC-123"
#define MAX_HTTP_RETRIES 3

// Configuración de la máquina de estados
#define PARKED_THRESHOLD_CYCLES 6 // 6 ciclos de 5 segundos = 30 segundos de motor apagado

static const char *TAG = "main";

void app_main(void) {
    // Inicializar memoria Flash para guardar credenciales permanentemente
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

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
        long timestamp_relativo = time_get_uptime_ms();
        buffer_push(&data, VEHICLE_ID, timestamp_relativo);

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
            
            ESP_LOGI(TAG, "Vehículo estacionado. Asegurando datos en Flash por seguridad...");
            buffer_persist_to_flash(); // Una sola escritura al finalizar el viaje.
            
            char saved_ssid[32] = {0};
            char saved_pass[64] = {0};
            int loaded_from_memory = (wifi_load_credentials(saved_ssid, saved_pass) == 0);

            // 1. INTENTO DE CAMINO FELIZ (Conectar a lo que sabemos)
            if (loaded_from_memory) {
                ESP_LOGI(TAG, "Intentando conectar a red guardada: %s", saved_ssid);
                wifi_connect(saved_ssid, saved_pass);
            }

            // 2. PLAN B: PORTAL CAUTIVO
            if (!wifi_is_connected()) {
                ESP_LOGW(TAG, "Fallo conexion a red conocida o no hay datos. Levantando Portal Cautivo...");
                
                wifi_start_ap("AutoLog-ABC123");
                httpd_handle_t web_server_handle = start_webserver();
                
                ESP_LOGI(TAG, "Esperando configuracion del celular (Timeout: 3 minutos)...");
                int wait_timeout = 0;
                
                // Espera hasta que lleguen datos o pasen 180 segundos
                while (!credentials_received && wait_timeout < 180) {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    wait_timeout++;
                }

                if (credentials_received) {
                    ESP_LOGI(TAG, "Cerrando AP e intentando conectar a la nueva red: %s", new_ssid);
                    if (web_server_handle) httpd_stop(web_server_handle);
                    wifi_stop_ap();
                    
                    // Intentamos conectar con los datos que escribió el usuario
                    if (wifi_connect(new_ssid, new_pass) == 0) {
                        ESP_LOGI(TAG, "¡Exito! Guardando credenciales a fuego...");
                        wifi_save_credentials(new_ssid, new_pass);
                    } else {
                        ESP_LOGE(TAG, "La clave nueva era incorrecta. El ciclo aborta para proteger batería.");
                    }
                } else {
                    ESP_LOGE(TAG, "Timeout del Portal Cautivo. Nadie se conecto.");
                    if (web_server_handle) httpd_stop(web_server_handle);
                    wifi_stop_ap();
                }
            }

            // 3. DESCARGA A LA NUBE (Solo entra si logramos conectarnos en el paso 1 o 2)
            if (wifi_is_connected()) {
                
                int max_intentos_generales = 10;
                int intento_actual = 0;

                while (!buffer_is_empty() && intento_actual < max_intentos_generales) {
                    
                    if (!wifi_is_connected()) wifi_connect(saved_ssid, saved_pass);

                    if (wifi_is_connected()) {
                        int items_to_send = buffer_count();
                        ESP_LOGI(TAG, "Descargando buffer: %d lecturas pendientes...", items_to_send);

                        char json_output[JSON_BUFFER_SIZE];
                        int successfully_sent = 0;

                        for (int i = 0; i < items_to_send; i++) {
                            const BufferedReading *reading = buffer_get(i);
                            if (build_json(&reading->data, reading->vehicle_id, reading->timestamp, json_output, JSON_BUFFER_SIZE) == 0) {
                                
                                int attempt = 0, sent_ok = 0;
                                while (attempt < MAX_HTTP_RETRIES && !sent_ok) {
                                    if (http_post_json(BACKEND_URL, json_output) == 0) sent_ok = 1; 
                                    else { attempt++; vTaskDelay(pdMS_TO_TICKS(1500)); }
                                }

                                if (sent_ok) { successfully_sent++; vTaskDelay(pdMS_TO_TICKS(1000)); } 
                                else { ESP_LOGW(TAG, "Servidor rechazo. Frenando ráfaga."); break; }
                            }
                        }

                        if (successfully_sent > 0) buffer_remove_first_n(successfully_sent);
                        if (buffer_is_empty()) break;
                        else vTaskDelay(pdMS_TO_TICKS(5000));
                    }
                    intento_actual++;
                }
            }

            // 4. A DORMIR PARA NO GASTAR BATERÍA DEL AUTO
            if (!buffer_is_empty()) {
                ESP_LOGE(TAG, "Protegiendo batería. Los datos no enviados quedan para el proximo viaje.");
            }
            
            ESP_LOGI(TAG, "Configurando alarma para despertar en 3 minutos...");
            esp_sleep_enable_timer_wakeup(180 * 1000000ULL);

            ESP_LOGI(TAG, "Apagando sistemas. Entrando en Deep Sleep...");
            if (wifi_is_connected()) wifi_disconnect(); 
            
            vTaskDelay(pdMS_TO_TICKS(1000)); 
            esp_deep_sleep_start(); 
        }

        // Frecuencia de muestreo mientras manejamos: 5 segundos.
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}