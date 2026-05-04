#include "time_manager.h"
#include "esp_log.h"
#include <time.h>

// ============================================================
// SWITCH DE ENTORNO: 1 para QEMU, 0 para ESP32 físico
#define QEMU_SIMULATION_MODE 1 
// ============================================================

#if !QEMU_SIMULATION_MODE
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static const char *TAG = "time_manager";
static int synced = 0;

#if !QEMU_SIMULATION_MODE
static void time_sync_callback(struct timeval *tv) {
    ESP_LOGI(TAG, "Hora sincronizada con NTP");
    synced = 1;
}
#endif

int time_sync(void) {
#if QEMU_SIMULATION_MODE
    ESP_LOGI(TAG, "Simulando sincronización NTP...");
    synced = 1;
    return 0;
#else
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_callback);
    esp_sntp_init();

    int retries = 0;
    while (!synced && retries < 20) {
        ESP_LOGI(TAG, "Esperando sincronización NTP...");
        vTaskDelay(pdMS_TO_TICKS(500));
        retries++;
    }

    if (!synced) {
        ESP_LOGE(TAG, "No se pudo sincronizar la hora");
        return -1;
    }
    return 0;
#endif
}

long time_get_timestamp(void) {
    if (!synced) {
        ESP_LOGW(TAG, "Hora no sincronizada, timestamp = 0");
        return 0;
    }

#if QEMU_SIMULATION_MODE
    // Devolvemos un timestamp de prueba fijo (ej. 1713374200)
    // Opcionalmente podrías sumar un contador para ver cómo avanza
    static long fake_time = 1713374200;
    fake_time += 5; // Simula que pasan 5 segundos por cada lectura
    return fake_time;
#else
    time_t now;
    time(&now);
    return (long)now;
#endif
}

int time_is_synced(void) {
    return synced;
}