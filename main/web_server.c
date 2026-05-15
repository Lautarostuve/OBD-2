#include "web_server.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "web_server";

char new_ssid[32] = "";
char new_pass[64] = "";
int credentials_received = 0;

const char* html_form = 
    "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>body{font-family:Arial; padding:20px; background:#f4f4f4;} "
    "input{width:100%; padding:10px; margin:10px 0;}</style></head><body>"
    "<h2>Configurar AutoLog</h2>"
    "<form action='/config' method='POST'>"
    "<label>Nombre de la red Wi-Fi:</label><br>"
    "<input type='text' name='ssid'><br>"
    "<label>Contrasena:</label><br>"
    "<input type='password' name='password'><br>"
    "<input type='submit' value='Guardar y Conectar' style='background:#007BFF; color:white; border:none;'>"
    "</form></body></html>";

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_form, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t config_post_handler(httpd_req_t *req) {
    char buf[128];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    // Parseo rústico de "ssid=MiRed&password=MiClave"
    char *ssid_ptr = strstr(buf, "ssid=");
    char *pass_ptr = strstr(buf, "&password=");
    
    if (ssid_ptr && pass_ptr) {
        ssid_ptr += 5; // Salta "ssid="
        *pass_ptr = '\0'; // Corta el string antes del &
        pass_ptr += 10; // Salta "&password="
        
        strncpy(new_ssid, ssid_ptr, sizeof(new_ssid) - 1);
        strncpy(new_pass, pass_ptr, sizeof(new_pass) - 1);
        
        ESP_LOGI(TAG, "Credenciales recibidas desde el celular!");
        credentials_received = 1;
    }

    const char* resp = "<html><body><h2>Datos guardados.</h2><p>El ESP32 intentara conectarse. Podes cerrar esta pagina.</p></body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_handle_t start_webserver(void) {
    credentials_received = 0; // Reseteamos bandera
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_get = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
        httpd_register_uri_handler(server, &uri_get);

        httpd_uri_t uri_post = { .uri = "/config", .method = HTTP_POST, .handler = config_post_handler };
        httpd_register_uri_handler(server, &uri_post);
    }
    return server;
}