#include "time_manager.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>

static const char *TAG = "time_manager";
static int synced = 0;

// ============================================================
// Callback que se llama automáticamente cuando el ESP32
// recibe la hora del servidor NTP y la sincroniza.
// ============================================================
static void time_sync_callback(struct timeval *tv) {
    ESP_LOGI(TAG, "Hora sincronizada con NTP");
    synced = 1;
}

int time_sync(void) {

    // Configurar el servidor NTP a usar.
    // pool.ntp.org es un servidor público y gratuito.
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");

    // Registrar el callback que se llama cuando llega la hora
    sntp_set_time_sync_notification_cb(time_sync_callback);

    // Arrancar el servicio SNTP
    esp_sntp_init();

    // Esperar hasta que se sincronice, con timeout de 10 segundos
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
}

long time_get_timestamp(void) {
    if (!synced) {
        ESP_LOGW(TAG, "Hora no sincronizada, timestamp = 0");
        return 0;
    }

    // time() devuelve los segundos transcurridos desde
    // el 1 de enero de 1970 (Unix time), que es exactamente
    // lo que necesitamos para el JSON
    time_t now;
    time(&now);
    return (long)now;
}

int time_is_synced(void) {
    return synced;
}