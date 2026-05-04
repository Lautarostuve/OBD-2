#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

// Sincroniza la hora con un servidor NTP
// Devuelve 0 si se sincronizó bien, -1 si falló
int time_sync(void);

// Devuelve el timestamp actual en segundos (Unix time)
// Devuelve 0 si todavía no se sincronizó
long time_get_timestamp(void);

// Devuelve 1 si la hora ya fue sincronizada, 0 si no
int time_is_synced(void);

#endif