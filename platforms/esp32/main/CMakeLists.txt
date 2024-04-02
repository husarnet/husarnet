idf_component_register(SRCS "test_runner.c" "webserver.c"
                    INCLUDE_DIRS ""
                    REQUIRES husarnet_core esp_wifi esp_http_server)