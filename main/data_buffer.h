#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H

#include "pid_parser.h"

// Maximo de lecturas que se pueden acumular
#define BUFFER_MAX_SIZE 10

// Una lectura completa con timestamp
typedef struct {
    EngineData data;
    long timestamp;
    char vehicle_id[16];
} BufferedReading;

// Agrega una lectura al buffer
// Devuelve 0 si se agregó, -1 si el buffer está lleno
int buffer_push(const EngineData *data, const char *vehicle_id, long timestamp);

// Devuelve la cantidad de lecturas acumuladas
int buffer_count(void);

// Devuelve 1 si el buffer está lleno, 0 si no
int buffer_is_full(void);

// Devuelve 1 si el buffer está vacio, 0 si no
int buffer_is_empty(void);

// Obtiene una lectura por índice (para iterar y mandarlas)
const BufferedReading *buffer_get(int index);

// Limpia el buffer después de mandar los datos
void buffer_clear(void);

#endif