#include "json_builder.h"
#include <stdio.h>

int build_json(const EngineData *data, const char *vehicle_id,
               long timestamp_relativo, char *output_buffer, int buffer_size) {

    if (data == NULL || vehicle_id == NULL || output_buffer == NULL) {
        return -1;
    }

    // Cambiamos "timestamp" por "timestamp_relativo"
    int written = snprintf(output_buffer, buffer_size,
        "{"
            "\"id_vehiculo\":\"%s\","
            "\"timestamp_relativo\":%ld,"
            "\"data\":{"
                "\"speed\":%.2f,"
                "\"rpm\":%.2f,"
                "\"temp\":%.2f,"
                "\"fuel_level\":%.2f"
            "}"
        "}",
        vehicle_id,
        timestamp_relativo,
        data->speed,
        data->rpm,
        data->temperature,
        data->fuel_level
    );

    // Verificar que no se cortó por falta de espacio
    if (written < 0 || written >= buffer_size) {
        return -1;
    }

    return 0;
}