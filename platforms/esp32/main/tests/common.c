// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "common.h"
#include <ctype.h>

// Version of esp_unity_readline that does not echo characters
void esp_read_line_no_echo(char* dst, size_t len) {
    /* Read line from console with support for echoing and backspaces */
    size_t write_index = 0;
    for (;;) {
        char c = 0;
        int result = fgetc(stdin);
        if (result == EOF) {
          continue;
        }
        c = (char) result;
        if (c == '\r' || c == '\n') {
            /* Add null terminator and return on newline */
            unity_putc('\n');
            dst[write_index] = '\0';
            return;
        } else if (c == '\b') {
            if (write_index > 0) {
                /* Delete previously entered character */
                write_index--;
            }
        } else if (len > 0 && write_index < len - 1 && !iscntrl(c)) {
            /* Write a max of len - 1 characters to allow for null terminator */
            dst[write_index++] = c;
        }
    }
}

void husarnet_enable_extended_logging() {
  esp_log_level_set("husarnet", ESP_LOG_INFO);
}

void husarnet_disable_extended_logging() {
  esp_log_level_set("husarnet", ESP_LOG_WARN);
}
