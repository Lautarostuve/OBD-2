#include "data_buffer.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "data_buffer";

// El buffer interno: un array de lecturas
static BufferedReading readings[BUFFER_MAX_SIZE];
static int count = 0;

int buffer_push(const EngineData *data, const char *vehicle_id, long timestamp) {
    if (count >= BUFFER_MAX_SIZE) {
        ESP_LOGW(TAG, "Buffer lleno, lectura descartada");
        return -1;
    }

    readings[count].data      = *data;
    readings[count].timestamp = timestamp;
    strncpy(readings[count].vehicle_id, vehicle_id, sizeof(readings[count].vehicle_id) - 1);

    count++;
    ESP_LOGI(TAG, "Lectura guardada (%d/%d)", count, BUFFER_MAX_SIZE);
    return 0;
}

int buffer_count(void) {
    return count;
}

int buffer_is_full(void) {
    return count >= BUFFER_MAX_SIZE;
}

int buffer_is_empty(void) {
    return count == 0;
}

const BufferedReading *buffer_get(int index) {
    if (index < 0 || index >= count) {
        return NULL;
    }
    return &readings[index];
}

void buffer_clear(void) {
    count = 0;
    ESP_LOGI(TAG, "Buffer vaciado");
}