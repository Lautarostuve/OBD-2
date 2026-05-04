#include "can_driver.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ============================================================
// SWITCH DE ENTORNO: Cambiar a 0 cuando tengas el ESP32 real
#define QEMU_SIMULATION_MODE 1 
// ============================================================

static const char *TAG = "can_driver";

// Variable para recordar qué PID nos pidió el main.c
static uint8_t last_requested_pid = 0;

// Pines estándar (ajustalos según tu placa final)
#define TX_PIN GPIO_NUM_5
#define RX_PIN GPIO_NUM_4

// Direcciones estándar de OBD-II
#define OBD2_REQUEST_ID  0x7DF
#define OBD2_RESPONSE_ID 0x7E8

int can_init(void) {
#if QEMU_SIMULATION_MODE
    ESP_LOGW(TAG, "Iniciando CAN en MODO SIMULADOR QEMU");
    return 0; // Fingimos que se instaló bien
#else
    ESP_LOGI(TAG, "Inicializando controlador TWAI físico a 500kbps...");

    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_PIN, RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); // Dejamos pasar todo para debug

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        ESP_LOGE(TAG, "Error: No se pudo instalar el driver TWAI");
        return -1;
    }

    if (twai_start() != ESP_OK) {
        ESP_LOGE(TAG, "Error: No se pudo arrancar el driver TWAI");
        return -1;
    }

    ESP_LOGI(TAG, "Driver TWAI físico iniciado correctamente");
    return 0;
#endif
}

int can_request_pid(uint8_t pid) {
#if QEMU_SIMULATION_MODE
    last_requested_pid = pid; // El auto "escucha" la pregunta
    // En QEMU no enviamos nada real porque no hay quién responda el ACK
    vTaskDelay(pdMS_TO_TICKS(10));
    return 0;
#else
    twai_message_t message;
    message.identifier = OBD2_REQUEST_ID;
    message.extd = 0; // Usamos IDs de 11 bits (Estándar CAN 2.0A)
    message.rtr = 0;
    message.data_length_code = 8; // La trama ISO-TP siempre mide 8 bytes

    // Armado del "Single Frame" de ISO-TP
    message.data[0] = 0x02; // Tamaño de la info: 2 bytes útiles
    message.data[1] = 0x01; // Modo OBD2 (01 = Current Data)
    message.data[2] = pid;  // El PID que queremos consultar
    
    // Relleno (Padding) para cumplir los 8 bytes
    for (int i = 3; i < 8; i++) message.data[i] = 0x55;

    // Intentamos transmitir. Si falla tras 100ms, hay un error en el cableado o no hay auto.
    if (twai_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
        return 0;
    }
    return -1;
#endif
}

int can_receive_data(uint8_t *out_pid, uint8_t *out_A, uint8_t *out_B) {
#if QEMU_SIMULATION_MODE
    *out_pid = last_requested_pid; // Respondemos al PID que nos preguntaron

    switch (last_requested_pid) {
        case 0x0C: // RPM -> simulamos 2500 RPM (0x27 * 256 + 0x10) / 4
            *out_A = 0x27;
            *out_B = 0x10;
            break;
        case 0x0D: // Velocidad -> simulamos 90 km/h (0x5A = 90)
            *out_A = 0x5A;
            *out_B = 0x00;
            break;
        case 0x05: // Temperatura -> simulamos 85 °C (0x7D = 125. 125 - 40 = 85)
            *out_A = 0x7D;
            *out_B = 0x00;
            break;
        case 0x2F: // Nivel de Combustible -> simulamos ~45% (0x73 = 115. (115/255)*100 = 45.09)
            *out_A = 0x73;
            *out_B = 0x00;
            break;
        default:
            *out_A = 0x00;
            *out_B = 0x00;
            break;
    }

    vTaskDelay(pdMS_TO_TICKS(50)); // Latencia de la ECU
    return 0;
#else
    twai_message_t message;
    // Esperamos la respuesta. Si el auto no responde en 100ms, damos timeout.
    if (twai_receive(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
        // ¿Nos está hablando el motor?
        if (message.identifier == OBD2_RESPONSE_ID) {
            // Verificamos si es una respuesta afirmativa (Modo 01 + 0x40 = 0x41)
            if (message.data[1] == 0x41) {
                *out_pid = message.data[2]; // Byte del PID
                *out_A   = message.data[3]; // Primer dato útil
                *out_B   = message.data[4]; // Segundo dato útil (si existe)
                return 0;
            }
        }
    }
    return -1;
#endif
}