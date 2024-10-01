#include <driver/uart.h>
#include "hardwareprofile.h"
#include <esp_log.h>
#include "rs232.h"


#define UART_PORTNUM   1
#define ECHO_READ_TOUT (3)     // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks
#define TIMEOUT_MS     50


static const char *TAG = "Machine serial";


void bsp_rs232_init(void) {
    uart_config_t uart_config = {
        .baud_rate           = 230400,
        .data_bits           = UART_DATA_8_BITS,
        .parity              = UART_PARITY_DISABLE,
        .stop_bits           = UART_STOP_BITS_1,
        .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_PORTNUM, &uart_config));

    uart_set_pin(UART_PORTNUM, BSP_HAP_TX, BSP_HAP_RX, -1, -1);
    ESP_ERROR_CHECK(uart_driver_install(UART_PORTNUM, 512, 512, 10, NULL, 0));
    ESP_ERROR_CHECK(uart_set_mode(UART_PORTNUM, UART_MODE_UART));
    ESP_ERROR_CHECK(uart_set_rx_timeout(UART_PORTNUM, ECHO_READ_TOUT));

    ESP_LOGI(TAG, "Initialized");
}


void bsp_rs232_flush(void) {
    uart_flush(UART_PORTNUM);
}


int bsp_rs232_read(uint8_t *buffer, size_t len) {
    return uart_read_bytes(UART_PORTNUM, buffer, len, pdMS_TO_TICKS(TIMEOUT_MS));
}


int bsp_rs232_write(uint8_t *buffer, size_t len) {
    return uart_write_bytes(UART_PORTNUM, buffer, len);
}
