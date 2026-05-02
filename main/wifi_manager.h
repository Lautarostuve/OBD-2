#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

// Devuelve 0 si se conectó, -1 si falló
int wifi_connect(const char *ssid, const char *password);

// Devuelve 1 si hay conexión activa, 0 si no
int wifi_is_connected(void);

void wifi_disconnect(void);

#endif