#include <time.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cJSON.h"
#include "esp_system.h"
//#include "esp_spi_flash.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_littlefs.h"
#include "esp_vfs_fat.h"
#include "fs_storage.h"


static const char *TAG = "FS Storage";


void fs_storage_mount_littlefs(void) {
    /* Print chip information */
    ESP_LOGI(TAG, "Initializing LittleFS");

    esp_vfs_littlefs_conf_t conf = {
        .base_path              = LITTLEFS_PARTITION_PATH,
        .partition_label        = LITTLEFS_PARTITION_LABEL,
        .format_if_mount_failed = true,
        .dont_mount             = false,
    };

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %zu, used: %zu", total, used);
    }
}


void fs_storage_umount_littlefs(void) {
    // All done, unmount partition and disable LittleFS
    esp_vfs_littlefs_unregister(LITTLEFS_PARTITION_LABEL);
    ESP_LOGI(TAG, "LittleFS unmounted");
}
