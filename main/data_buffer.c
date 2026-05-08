#include "data_buffer.h"
#include "esp_log.h"
#include "esp_partition.h"
#include <string.h>

static const char *TAG = "data_buffer";

typedef struct {
    uint32_t magic_start;  // 0x0BD20001
    int count;
    uint32_t magic_end;    // 0x0BD20002 - confirma que la escritura fue completa
} BufferHeader;

#define MAGIC_START   0x0BD20001
#define MAGIC_END     0x0BD20002
#define HEADER_OFFSET 0
#define DATA_OFFSET   4096  // Los datos empiezan después del primer sector (4KB)

static const esp_partition_t *partition = NULL;
static int count = 0;

// Borra el sector del header y lo reescribe de forma segura
static void write_header(int new_count) {
    esp_partition_erase_range(partition, 0, 4096);
    BufferHeader header = {
        .magic_start = MAGIC_START,
        .count = new_count,
        .magic_end = MAGIC_END
    };
    esp_partition_write(partition, HEADER_OFFSET, &header, sizeof(header));
}

int buffer_init(void) {
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x99, "obd_data");
    if (partition == NULL) {
        ESP_LOGE(TAG, "No se encontró la partición obd_data");
        return -1;
    }

    BufferHeader header;
    esp_partition_read(partition, HEADER_OFFSET, &header, sizeof(header));

    if (header.magic_start == MAGIC_START && header.magic_end == MAGIC_END) {
        count = header.count;
        ESP_LOGI(TAG, "Buffer recuperado: %d lecturas previas encontradas", count);

        // LOG TEMPORAL: mostrar las primeras 3 lecturas recuperadas
        for (int i = 0; i < count && i < 3; i++) {
            const BufferedReading *r = buffer_get(i);
            ESP_LOGI(TAG, "  [%d] ts=%ld speed=%.2f rpm=%.2f temp=%.2f fuel=%.2f",
                i, r->timestamp, r->data.speed, r->data.rpm, 
                r->data.temperature, r->data.fuel_level);
        }
        
    } else {
        ESP_LOGI(TAG, "Inicializando partición nueva...");
        esp_partition_erase_range(partition, 0, partition->size);
        BufferHeader nuevo = { .magic_start = MAGIC_START, .count = 0, .magic_end = MAGIC_END };
        esp_partition_write(partition, HEADER_OFFSET, &nuevo, sizeof(nuevo));
        count = 0;
    }
    return 0;
}

int buffer_push(const EngineData *data, const char *vehicle_id, long timestamp) {
    if (partition == NULL) return -1;

    if (count >= BUFFER_MAX_SIZE) {
        ESP_LOGW(TAG, "Buffer lleno, lectura descartada");
        return -1;
    }

    // Construir la lectura
    BufferedReading reading;
    reading.data = *data;
    reading.timestamp = timestamp;
    strncpy(reading.vehicle_id, vehicle_id, sizeof(reading.vehicle_id) - 1);

    // Escribir la lectura en su posición
    uint32_t offset = DATA_OFFSET + (count * sizeof(BufferedReading));
    esp_partition_write(partition, offset, &reading, sizeof(reading));

    // Actualizar el header de forma segura
    count++;
    write_header(count);

    ESP_LOGI(TAG, "Lectura guardada (%d/%d)", count, BUFFER_MAX_SIZE);
    return 0;
}

const BufferedReading *buffer_get(int index) {
    if (partition == NULL || index < 0 || index >= count) return NULL;

    static BufferedReading temp;
    uint32_t offset = DATA_OFFSET + (index * sizeof(BufferedReading));
    esp_partition_read(partition, offset, &temp, sizeof(temp));
    return &temp;
}

void buffer_remove_first_n(int n) {
    if (partition == NULL || n <= 0) return;

    if (n >= count) {
        buffer_clear();
        return;
    }

    // Leer los elementos restantes y reescribirlos desde el inicio
    int remaining = count - n;
    for (int i = 0; i < remaining; i++) {
        BufferedReading temp;
        uint32_t src = DATA_OFFSET + ((i + n) * sizeof(BufferedReading));
        esp_partition_read(partition, src, &temp, sizeof(temp));

        if (i == 0) {
            // Borrar toda la zona de datos antes de reescribir
            esp_partition_erase_range(partition, DATA_OFFSET, partition->size - DATA_OFFSET);
        }

        uint32_t dst = DATA_OFFSET + (i * sizeof(BufferedReading));
        esp_partition_write(partition, dst, &temp, sizeof(temp));
    }

    count = remaining;
    write_header(count);

    ESP_LOGI(TAG, "Buffer actualizado: %d enviados, %d restantes", n, count);
}

void buffer_clear(void) {
    if (partition == NULL) return;
    esp_partition_erase_range(partition, DATA_OFFSET, partition->size - DATA_OFFSET);
    count = 0;
    write_header(0);
    ESP_LOGI(TAG, "Buffer vaciado");
}

int buffer_count(void) { return count; }
int buffer_is_full(void)  { return count >= BUFFER_MAX_SIZE; }
int buffer_is_empty(void) { return count == 0; }