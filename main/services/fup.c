#ifndef BUILD_CONFIG_SIMULATED_APP
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <nvs_flash.h>
#include <esp_system.h>
#endif
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <esp_log.h>
#include "bsp/msc.h"
#include "fup.h"
#include "config/app_config.h"
#include "model/model.h"
#include "services/timestamp.h"


#define FUP_BIN (MSC_USB_MOUNTPOINT "/lavatrice-display-eta-beta-rotondi.bin")


typedef enum {
    FUP_INTERNAL_STATE_READY = 0,
    FUP_INTERNAL_STATE_STARTED,
    FUP_INTERNAL_STATE_BEGIN,
    FUP_INTERNAL_STATE_WRITE,
    FUP_INTERNAL_STATE_END,
    FUP_INTERNAL_STATE_FAILED,
    FUP_INTERNAL_STATE_SUCCESS,
} fup_internal_state_t;


static const char *TAG = "FUP";
#ifndef BUILD_CONFIG_SIMULATED_APP
static struct {
    fup_internal_state_t   internal_state;
    timestamp_t            ts;
    FILE                  *f;
    size_t                 size;
    const esp_partition_t *update_partition;
    esp_ota_handle_t       update_handle;
    size_t                 bytes_read;
    size_t                 total_bytes;
    uint8_t               *buffer;
} state = {0};
#endif


firmware_update_state_t fup_get_state(void) {
#ifndef BUILD_CONFIG_SIMULATED_APP
    switch (state.internal_state) {
        case FUP_INTERNAL_STATE_READY:
            if (access(FUP_BIN, F_OK) == 0) {
                return FIRMWARE_UPDATE_STATE_AVAILABLE;
            } else {
                return FIRMWARE_UPDATE_STATE_NONE;
            }
        case FUP_INTERNAL_STATE_FAILED:
            return FIRMWARE_UPDATE_STATE_FAILURE;
        case FUP_INTERNAL_STATE_SUCCESS:
            return FIRMWARE_UPDATE_STATE_SUCCESS;
        default:
            return FIRMWARE_UPDATE_STATE_UPDATING;
    }
#endif
    return FIRMWARE_UPDATE_STATE_NONE;
}


uint8_t fup_proceed(void) {
#ifndef BUILD_CONFIG_SIMULATED_APP
    switch (state.internal_state) {
        case FUP_INTERNAL_STATE_READY:
        case FUP_INTERNAL_STATE_FAILED:
            state.internal_state = FUP_INTERNAL_STATE_STARTED;
            state.ts             = timestamp_get();
            break;
        default:
            break;
    }
#endif
    return 0;
}


void fup_manage(void) {
    (void)TAG;
#ifndef BUILD_CONFIG_SIMULATED_APP
    const size_t buffer_size = 2048;

    switch (state.internal_state) {
        case FUP_INTERNAL_STATE_READY:
            break;
        case FUP_INTERNAL_STATE_STARTED: {
            // Wait a bit to allow the UI to updat<Plug>(coc-definition)e
            if (!timestamp_is_expired(state.ts, 100)) {
                break;
            }

            state.f = fopen(FUP_BIN, "r");
            if (state.f == NULL) {
                ESP_LOGE(TAG, "Unable to open %s: %s", FUP_BIN, strerror(errno));
                state.internal_state = FUP_INTERNAL_STATE_FAILED;
                break;
            }

            state.update_partition = esp_ota_get_next_update_partition(NULL);
            if (state.update_partition == NULL) {
                ESP_LOGE(TAG, "esp_ota_get_next_update_partition failed!");
                fclose(state.f);

                state.internal_state = FUP_INTERNAL_STATE_FAILED;
                break;
            } else {
                ESP_LOGI(TAG, "Next partition %s", state.update_partition->label);
            }

            fseek(state.f, 0L, SEEK_END);
            state.size = ftell(state.f);
            rewind(state.f);

            state.internal_state = FUP_INTERNAL_STATE_BEGIN;
            break;
        }

        case FUP_INTERNAL_STATE_BEGIN: {
            ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%" PRIXLEAST32 ", update size %zu",
                     state.update_partition->subtype, state.update_partition->address, state.size);

            // The following call takes about 1000ms, it should be made async (or buffered)
            esp_err_t err = ESP_OK;
            if ((err = esp_ota_begin(state.update_partition, OTA_SIZE_UNKNOWN, &state.update_handle) != ESP_OK)) {
                ESP_LOGE(TAG, "esp_ota_begin failed (0x%04X)!", err);
                fclose(state.f);

                state.internal_state = FUP_INTERNAL_STATE_FAILED;
                break;
            } else {
                ESP_LOGI(TAG, "OTA Begun");
            }

            state.bytes_read = 0;
            state.buffer     = malloc(buffer_size);
            if (state.buffer == NULL) {
                ESP_LOGE(TAG, "Unable to allocate memory!");

                state.internal_state = FUP_INTERNAL_STATE_FAILED;
                break;
            }

            state.internal_state = FUP_INTERNAL_STATE_WRITE;
            break;
        }

        case FUP_INTERNAL_STATE_WRITE: {
            esp_err_t err = ESP_OK;

            if ((state.bytes_read = fread(state.buffer, 1, buffer_size, state.f)) > 0) {
                if ((err = esp_ota_write(state.update_handle, state.buffer, state.bytes_read)) != ESP_OK) {
                    ESP_LOGE(TAG, "esp_ota_write failed (0x%04X)!", err);
                    fclose(state.f);
                    free(state.buffer);

                    state.internal_state = FUP_INTERNAL_STATE_FAILED;
                    break;
                } else {
                    state.total_bytes += state.bytes_read;
                    ESP_LOGI(TAG, "Update: [%8zu/%8zu]", state.total_bytes, state.size);
                }
            } else {
                free(state.buffer);
                fclose(state.f);
                state.internal_state = FUP_INTERNAL_STATE_END;
            }
            break;
        }

        case FUP_INTERNAL_STATE_END: {
            esp_err_t err = ESP_OK;
            if ((err = esp_ota_end(state.update_handle)) != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_end failed (0x%04X)!", err);

                state.internal_state = FUP_INTERNAL_STATE_FAILED;
                break;
            }

            if ((ESP_OK == err) && (err = esp_ota_set_boot_partition(state.update_partition) != ESP_OK)) {
                ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (0x%04X)!", err);

                state.internal_state = FUP_INTERNAL_STATE_FAILED;
                break;
            }

            ESP_LOGI(TAG, "Update successful!");
            state.internal_state = FUP_INTERNAL_STATE_SUCCESS;
            break;
        }

        case FUP_INTERNAL_STATE_SUCCESS:
            break;

        case FUP_INTERNAL_STATE_FAILED:
            break;
    }
#endif
}
