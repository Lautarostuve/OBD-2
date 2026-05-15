#include "time_manager.h"
#include "esp_timer.h" // Librería nativa para el reloj interno del chip

long time_get_uptime_ms(void) {
    // esp_timer_get_time() devuelve microsegundos desde el arranque.
    // Lo dividimos por 1000 para enviarlo como milisegundos estándar.
    return (long)(esp_timer_get_time() / 1000ULL);
}