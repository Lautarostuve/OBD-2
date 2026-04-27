#ifndef JSON_BUILDER_H
#define JSON_BUILDER_H

#include "pid_parser.h"

// Tamaño maximo del JSON generado
#define JSON_BUFFER_SIZE 256

// Genera el JSON y lo guarda en el buffer
// Devuelve 0 si todo salio bien, -1 si hubo error
int build_json(const EngineData *data, const char *vehicle_id, 
               long timestamp, char *output_buffer, int buffer_size);

#endif