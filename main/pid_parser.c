#include "pid_parser.h"

// Velocidad: el valor llega directo, sin fórmula
float parse_speed(uint8_t A) {
    return (float)A; // km/h
}

// RPM: fórmula estándar OBD-II → (A * 256 + B) / 4
float parse_rpm(uint8_t A, uint8_t B) {
    return ((A * 256.0f) + B) / 4.0f;
}

// Temperatura: fórmula estándar OBD-II → A - 40
float parse_temperature(uint8_t A) {
    return (float)A - 40.0f; // °C
}

// Nivel de combustible: fórmula estándar OBD-II → (A / 255) * 100
float parse_fuel_level(uint8_t A) {
    return (A / 255.0f) * 100.0f; // %
}

// Función central: según el PID recibido, aplica la fórmula correcta
void parse_pid(uint8_t pid, uint8_t A, uint8_t B, EngineData *data) {
    switch (pid) {
        case 0x0D:
            data->speed = parse_speed(A);  //como en python, data.speed = data->speed. guarda el resultado de la función parse_speed en el campo speed de la estructura data.
            break;
        case 0x0C:
            data->rpm = parse_rpm(A, B);
            break;
        case 0x05:
            data->temperature = parse_temperature(A);
            break;
        case 0x2F:
            data->fuel_level = parse_fuel_level(A);
            break;
        default:
            break;
    }
}