#include "json_builder.h"
#include <stdio.h>

int build_json(const EngineData *data, const char *vehicle_id,
               long timestamp, char *output_buffer, int buffer_size) {

    if (data == NULL || vehicle_id == NULL || output_buffer == NULL) {
        return -1;
    }

    int written = snprintf(output_buffer, buffer_size,
        "{"
            "\"id_vehiculo\":\"%s\","
            "\"timestamp\":%ld,"
            "\"data\":{"
                "\"speed\":%.2f,"
                "\"rpm\":%.2f,"
                "\"temp\":%.2f,"
                "\"fuel_level\":%.2f"
            "}"
        "}",
        vehicle_id,
        timestamp,
        data->speed,
        data->rpm,
        data->temperature,
        data->fuel_level
    );

    // Verificar que no se corto por falta de espacio
    if (written < 0 || written >= buffer_size) {
        return -1;
    }

    return 0;
}