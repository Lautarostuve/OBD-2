#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include <stdint.h>

// Inicializa los pines TX/RX y configura el bus a 500 kbps
int can_init(void);

// Envia la solicitud al motor (ID: 0x7DF) para un PID específico
int can_request_pid(uint8_t pid);

// Escucha la respuesta del motor (ID: 0x7E8)
// Devuelve 0 si leyó un dato válido, o -1 si hubo timeout/error
int can_receive_data(uint8_t *out_pid, uint8_t *out_A, uint8_t *out_B);

#endif