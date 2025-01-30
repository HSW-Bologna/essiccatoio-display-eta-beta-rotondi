#include "system.h"
#include "esp_log.h"
#include "hardwareprofile.h"
#include <driver/spi_master.h>
#include <driver/i2c.h>


static const char *TAG                   = "System";
static const int   SPI_MAX_TRANSFER_SIZE = 32768;


void bsp_system_init(void) {
    ESP_LOGI(TAG, "Initializing SPI bus (MOSI:%d, MISO:%d, CLK:%d)", BSP_HAP_DOUT_D, GPIO_NUM_NC, BSP_HAP_CLK_D);
    spi_bus_config_t bus = {
        .mosi_io_num     = BSP_HAP_DOUT_D,
        .miso_io_num     = GPIO_NUM_NC,
        .sclk_io_num     = BSP_HAP_CLK_D,
        .quadwp_io_num   = GPIO_NUM_NC,
        .quadhd_io_num   = GPIO_NUM_NC,
        .data4_io_num    = GPIO_NUM_NC,
        .data5_io_num    = GPIO_NUM_NC,
        .data6_io_num    = GPIO_NUM_NC,
        .data7_io_num    = GPIO_NUM_NC,
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
        .flags = SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MOSI | SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_config_t i2c_conf = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = BSP_HAP_SDA,
        .scl_io_num       = BSP_HAP_SCL,
        .sda_pullup_en    = GPIO_PULLUP_DISABLE,
        .scl_pullup_en    = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 100 * 1000,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

    ESP_LOGI(TAG, "Initialized");
}


void bsp_system_reset(void) {
    esp_restart();
}
