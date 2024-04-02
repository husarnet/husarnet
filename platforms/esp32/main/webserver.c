#include "esp_log.h"
#include "esp_http_server.h"

static const char *TAG = "webserver";

static esp_err_t root_get_handler(httpd_req_t *req) {
    const char resp[] = "Hello, Husarnet!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void start_webserver() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;

  ESP_LOGI(TAG, "Starting server on port %d", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_uri_t root = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &root);
  }
}