#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pid_parser.h"
#include "json_builder.h"

void app_main(void) {

    EngineData data = {0};

    parse_pid(0x0C, 0x1F, 0x40, &data);
    parse_pid(0x0D, 0x5A, 0x00, &data);
    parse_pid(0x05, 0x7D, 0x00, &data);
    parse_pid(0x2F, 0x72, 0x00, &data);

    char json_output[JSON_BUFFER_SIZE];
    int result = build_json(&data, "ABC-123", 1713374200,
                            json_output, JSON_BUFFER_SIZE);

    if (result == 0) {
        printf("JSON generado:\n%s\n", json_output);
    } else {
        printf("Error al generar el JSON\n");
    }

    // Mantener el programa vivo para que se vea el output
    while(1) {
        printf("Esperando...\n");
        vTaskDelay(pdMS_TO_TICKS(100000));
    }
}