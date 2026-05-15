#include "data_buffer.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_sleep.h"
#include <string.h>

static const char *TAG = "data_buffer";

// ============================================================
// MEMORIA RTC: Sobrevive al Deep Sleep, no tiene desgaste físico.
// Pero se borra si desconectás la batería del auto.
// ============================================================
#define BUFFER_MAX_SIZE 4000

static BufferedReading ram_buffer[BUFFER_MAX_SIZE]; 
static int ram_count = 0;

static const esp_partition_t *partition = NULL;

int buffer_init(void) {
    // Buscamos la partición en la Flash por si hay que recuperar algo de un viaje viejo
    partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x99, "obd_data");
    
    // Si despertamos de Deep Sleep, ram_count ya tiene su valor anterior.
    // Si es un "Cold Boot" (recién enchufado), ram_count será 0.
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) {
        ESP_LOGI(TAG, "Arranque en frío. El buffer RTC está vacío.");
        ram_count = 0;
    } else {
        ESP_LOGI(TAG, "Despertar de Sleep. Recuperadas %d lecturas de la RAM RTC.", ram_count);
    }
    
    return 0;
}

int buffer_push(const EngineData *data, const char *vehicle_id, long timestamp) {
    if (ram_count >= BUFFER_MAX_SIZE) {
        ESP_LOGW(TAG, "Buffer RTC lleno. Podrías llamar a una función para persistir en Flash aquí.");
        return -1;
    }

    // Guardamos en RAM (Velocidad instantánea, cero desgaste)
    ram_buffer[ram_count].data = *data;
    ram_buffer[ram_count].timestamp = timestamp;
    strncpy(ram_buffer[ram_count].vehicle_id, vehicle_id, sizeof(ram_buffer[ram_count].vehicle_id) - 1);

    ram_count++;
    ESP_LOGI(TAG, "Lectura guardada en RAM RTC (%d/%d)", ram_count, BUFFER_MAX_SIZE);
    return 0;
}

const BufferedReading *buffer_get(int index) {
    if (index < 0 || index >= ram_count) return NULL;
    return &ram_buffer[index];
}

// Nueva función: Guarda todo lo que hay en RAM a la Flash de un solo saque.
// Solo la llamarás cuando el auto se estacione (Fase 3).
void buffer_persist_to_flash(void) {
    if (partition == NULL || ram_count == 0) return;

    ESP_LOGI(TAG, "Guardando buffer de viaje a la Flash (Persistencia)...");
    esp_partition_erase_range(partition, 0, partition->size);
    
    // Escribimos todo el bloque de RAM a la Flash en una única operación
    esp_partition_write(partition, 4096, ram_buffer, ram_count * sizeof(BufferedReading));
    
    // Guardar el header (count)
    // (Aquí podrías reusar tu lógica de write_header anterior)
    ESP_LOGI(TAG, "Copia de seguridad en Flash completada.");
}

void buffer_remove_first_n(int n) {
    if (n <= 0) return;
    if (n >= ram_count) {
        ram_count = 0;
        return;
    }

    int remaining = ram_count - n;
    memmove(ram_buffer, &ram_buffer[n], remaining * sizeof(BufferedReading));
    ram_count = remaining;
    
    ESP_LOGI(TAG, "RAM RTC actualizada: %d enviados, %d restantes", n, ram_count);
}

int buffer_count(void) { return ram_count; }
int buffer_is_empty(void) { return ram_count == 0; }