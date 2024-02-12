idf_component_register(SRCS "main.c" "wifi.c" "webserver.c"
                    INCLUDE_DIRS ""
                    REQUIRES husarnet_core lwip esp_hw_support nvs_flash esp_netif esp_wifi esp_http_server)