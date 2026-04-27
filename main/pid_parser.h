#ifndef PID_PARSER_H
#define PID_PARSER_H

#include <stdint.h>

// Estructura que contiene todos los datos del motor
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