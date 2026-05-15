#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

// Devuelve 0 si se conectó, -1 si falló
int wifi_connect(const char *ssid, const char *password);

// Devuelve 1 si hay conexión activa, 0 si no
int wifi_is_connected(void);

void wifi_disconnect(void);

void wifi_start_ap(const char *ap_ssid);
void wifi_stop_ap(void);
int wifi_save_credentials(const char *ssid, const char *password);
int wifi_load_credentials(char *ssid, char *password);

#endif
