#ifndef PID_PARSER_H
#define PID_PARSER_H

#include <stdint.h>

// Estructura que contiene todos los datos del motor

// NOTA IMPORTANTE para cuando se integre con ISO-TP:
// La respuesta cruda del auto tiene el formato:
// [0] = cantidad de bytes utiles
// [1] = 0x41 (respuesta modo 01)
// [2] = PID consultado
// [3] = Byte A (primer dato)
// [4] = Byte B (segundo dato, usado en RPM)
// Ejemplo: 04 41 0C 1F 40 → RPM = (0x1F * 256 + 0x40) / 4 = 2000


typedef struct {
    float speed;        // km/h
    float rpm;          // revoluciones por minuto
    float temperature;  // °C
    float fuel_level;   // %
    int   check_engine; // 0 = ok, 1 = falla
} EngineData;

// Funciones de parseo por PID
float parse_speed(uint8_t A);
float parse_rpm(uint8_t A, uint8_t B);
float parse_temperature(uint8_t A);
float parse_fuel_level(uint8_t A);

// Función principal: recibe PID y bytes crudos, devuelve los datos parseados
void parse_pid(uint8_t pid, uint8_t A, uint8_t B, EngineData *data);

#endif