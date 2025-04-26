#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_RESPONSE_LENGTH 2048


void http_manager_init(void);
bool http_get(const char* url, char* resp, size_t* resp_len);