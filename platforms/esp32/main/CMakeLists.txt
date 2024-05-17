list(APPEND srcs
  "test_runner.c"
  "iperf_wrapper.c"
  "webserver.c"
  "tests/common.c"
  "tests/test_iperf.c"
  "tests/test_join.c"
  "tests/test_ping.c")

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    PRIV_INCLUDE_DIRS "."
                    REQUIRES unity test_utils husarnet_core esp_eth esp_http_server iperf
                    WHOLE_ARCHIVE)
