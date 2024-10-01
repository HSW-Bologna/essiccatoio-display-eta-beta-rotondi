#ifndef ESP_LOG_H_INCLUDED
#define ESP_LOG_H_INCLUDED

#include <stdio.h>

#define ESP_LOG_BUFFER_HEX(tag, buffer, len) printf("%s: buffer of size %i\n", tag, len)
#define ESP_LOGD(tag, format, ...)           
#define ESP_LOGI(tag, format, ...)           printf("%s: " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...)           printf("%s: " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...)           printf("%s: " format "\n", tag, ##__VA_ARGS__)

#endif
