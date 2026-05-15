#ifndef WEB_SERVER_H
#define WEB_SERVER_H
#include "esp_http_server.h"

extern char new_ssid[32];
extern char new_pass[64];
extern int credentials_received;

httpd_handle_t start_webserver(void);

#endif