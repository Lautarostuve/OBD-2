#ifndef HTTP_SENDER_H
#define HTTP_SENDER_H

// Envia el JSON por HTTP POST al backend
// Devuelve 0 si el servidor respondio 200 OK, -1 si hubo error
int http_post_json(const char *url, const char *json_body);

#endif