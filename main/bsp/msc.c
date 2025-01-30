#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "usb/usb_host.h"
#include "usb/msc_host.h"
#include "usb/msc_host_vfs.h"
#include "ffconf.h"
#include "esp_vfs.h"
#include "errno.h"
#include "driver/gpio.h"
#include <esp_vfs_fat.h>
#include "hardwareprofile.h"
#include "msc.h"
#include "controller/configuration/archive_management.h"


#define EVENT_READY_TO_UNINSTALL  (EVENT_HOST_NO_CLIENT | EVENT_HOST_ALL_FREE | EVENT_DRIVE_READY | EVENT_DRIVE_FAILED)
#define EVENT_HOST_NO_CLIENT      0x1
#define EVENT_HOST_ALL_FREE       0x2
#define EVENT_DEVICE_CONNECTED    0x4
#define EVENT_DEVICE_DISCONNECTED 0x8
#define EVENT_DEVICE_ADDRESS_MASK 0xFF0
#define EVENT_DRIVE_READY         0x1000
#define EVENT_DRIVE_FAILED        0x2000


typedef enum {
    TASK_MESSAGE_CODE_LOAD_ARCHIVE,
    TASK_MESSAGE_CODE_SAVE_ARCHIVE,
} task_message_code_t;


typedef struct {
    task_message_code_t code;
    union {
        name_t archive_name;
    };
} task_message_t;


static void    msc_task(void *args);
static uint8_t wait_for_msc_device(uint8_t *address, unsigned long delay_ms);
static bool    wait_for_event(EventBits_t event, TickType_t timeout);
static void    handle_usb_events(void *args);
static void    msc_event_cb(const msc_host_event_t *event, void *arg);
static void    utils_free_string_list(char **strings, size_t len);


static const char        *TAG = "MSC";
static EventGroupHandle_t usb_flags;
// static QueueHandle_t      message_queue;
static QueueHandle_t     response_queue;
static SemaphoreHandle_t sem;
static size_t            listed_archives_num = 0;
static char            **listed_archives     = NULL;


void msc_init(void) {
    const gpio_config_t input_pin = {
        .pin_bit_mask = BIT64(BSP_HAP_USB_INT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&input_pin));

    assert(sem == NULL);
    static StaticSemaphore_t static_semaphore;
    sem = xSemaphoreCreateMutexStatic(&static_semaphore);

    assert(usb_flags == NULL);
    static StaticEventGroup_t static_event_group;
    usb_flags = xEventGroupCreateStatic(&static_event_group);

    /*
    assert(message_queue == NULL);
    static uint8_t       message_queue_buffer[sizeof(task_message_t) * 4] = {0};
    static StaticQueue_t static_message_queue;
    message_queue = xQueueCreateStatic(sizeof(message_queue_buffer) / sizeof(task_message_t), sizeof(task_message_t),
                                       message_queue_buffer, &static_message_queue);
                                       */

    assert(response_queue == NULL);
    static uint8_t       response_queue_buffer[sizeof(msc_response_t) * 4] = {0};
    static StaticQueue_t static_response_queue;
    response_queue = xQueueCreateStatic(sizeof(response_queue_buffer) / sizeof(msc_response_t), sizeof(msc_response_t),
                                        response_queue_buffer, &static_response_queue);

    const usb_host_config_t host_config = {.intr_flags = ESP_INTR_FLAG_LOWMED};
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    xTaskCreate(handle_usb_events, TAG, 4096, NULL, 2, NULL);
    xTaskCreate(msc_task, TAG, 3072, NULL, 2, NULL);

    const msc_host_driver_config_t msc_config = {
        .create_backround_task = true,
        .task_priority         = 5,
        .stack_size            = 3072,
        .callback              = msc_event_cb,
    };
    ESP_ERROR_CHECK(msc_host_install(&msc_config));

    ESP_LOGI(TAG, "Initialized");
}


uint8_t msc_is_usb_mounted(void) {
    return (xEventGroupGetBits(usb_flags) & EVENT_DRIVE_READY) > 0;
}


removable_drive_state_t msc_is_device_mounted(void) {
    uint32_t res = xEventGroupWaitBits(usb_flags, EVENT_DRIVE_READY | EVENT_DRIVE_FAILED, pdFALSE, pdFALSE, 0);
    if ((res & EVENT_DRIVE_READY) > 0) {
        return REMOVABLE_DRIVE_STATE_MOUNTED;
    } else if ((res & EVENT_DRIVE_FAILED) > 0) {
        return REMOVABLE_DRIVE_STATE_INVALID;
    }
    return REMOVABLE_DRIVE_STATE_MISSING;
}


void msc_extract_archive(const name_t archive) {
    char string[128] = {0};
    snprintf(string, sizeof(string), "%s/%s%s", MSC_USB_MOUNTPOINT, archive, ARCHIVE_SUFFIX);
    msc_response_t response = {.code = MSC_RESPONSE_CODE_ARCHIVE_EXTRACTION_COMPLETE};
    response.error          = archive_management_extract_configuration(string);
    xQueueSend(response_queue, &response, portMAX_DELAY);

    /*
    task_message_t message = {.code = TASK_MESSAGE_CODE_LOAD_ARCHIVE};
    strcpy(message.archive_name, archive);
    xQueueSend(message_queue, &message, portMAX_DELAY);
    */
}


size_t msc_read_archives(model_t *pmodel) {
    if (msc_is_device_mounted() == REMOVABLE_DRIVE_STATE_MOUNTED) {
        xSemaphoreTake(sem, portMAX_DELAY);
        //pmodel->system.num_archivi = archive_management_copy_archive_names(listed_archives, &pmodel->system.archivi, listed_archives_num);
        xSemaphoreGive(sem);
        //return pmodel->system.num_archivi;
        return 0;
    } else {
        //pmodel->system.num_archivi = 0;
        //free(pmodel->system.archivi);
        //pmodel->system.archivi = NULL;
        return 0;
    }
}


void msc_save_archive(const name_t archive) {
    msc_response_t response = {.code = MSC_RESPONSE_CODE_ARCHIVE_SAVING_COMPLETE};
    response.error          = archive_management_save_configuration(MSC_USB_MOUNTPOINT, archive);
    xQueueSend(response_queue, &response, portMAX_DELAY);

    /*
task_message_t message = {.code = TASK_MESSAGE_CODE_SAVE_ARCHIVE};
strcpy(message.archive_name, archive);
xQueueSend(message_queue, &message, portMAX_DELAY);
*/
}


int msc_get_response(msc_response_t *response) {
    return xQueueReceive(response_queue, response, 0) == pdTRUE;
}


static void msc_event_cb(const msc_host_event_t *event, void *arg) {
    if (event->event == MSC_DEVICE_CONNECTED) {
        ESP_LOGI(TAG, "MSC device connected");
        // Obtained USB device address is placed after application events
        xEventGroupSetBits(usb_flags, EVENT_DEVICE_CONNECTED | (event->device.address << 4));
    } else if (event->event == MSC_DEVICE_DISCONNECTED) {
        xEventGroupSetBits(usb_flags, EVENT_DEVICE_DISCONNECTED);
        ESP_LOGI(TAG, "MSC device disconnected");
    } else {
        ESP_LOGI(TAG, "USB event %i", event->event);
    }
}


static void print_device_info(msc_host_device_info_t *info) {
    const size_t megabyte = 1024 * 1024;
    uint64_t     capacity = ((uint64_t)info->sector_size * info->sector_count) / megabyte;

    printf("Device info:\n");
    printf("\t Capacity: %" PRIu64 " MB\n", capacity);
    printf("\t Sector size: %" PRIu32 "\n", info->sector_size);
    printf("\t Sector count: %" PRIu32 "\n", info->sector_count);
    printf("\t PID: 0x%4X \n", info->idProduct);
    printf("\t VID: 0x%4X \n", info->idVendor);
    wprintf(L"\t iProduct: %S \n", info->iProduct);
    wprintf(L"\t iManufacturer: %S \n", info->iManufacturer);
    wprintf(L"\t iSerialNumber: %S \n", info->iSerialNumber);
}


// Handles common USB host library events
static void handle_usb_events(void *args) {
    for (;;) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);

        // Release devices once all clients has deregistered
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
            xEventGroupSetBits(usb_flags, EVENT_HOST_NO_CLIENT);
        }
        // Give ready_to_uninstall_usb semaphore to indicate that USB Host library
        // can be deinitialized, and terminate this task.
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            xEventGroupSetBits(usb_flags, EVENT_HOST_ALL_FREE);
        }
    }

    vTaskDelete(NULL);
}


static void msc_task(void *args) {
    (void)args;

    msc_host_device_handle_t msc_device;
    msc_host_vfs_handle_t    vfs_handle;
    msc_host_device_info_t   info;

    uint8_t usb_connected = 0;

    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 3,
        .allocation_unit_size   = 1024,
    };

    uint8_t old_id = -1;

    for (;;) {
        uint8_t device_address = 0;

        uint8_t current_id = gpio_get_level(BSP_HAP_USB_INT);
        if (old_id != current_id) {
            if (current_id == 0) {
                // A usb drive has been connected, turn on
                ESP_LOGI(TAG, "Chiavetta connessa");
            } else {
                // No drive connected, turn off
                ESP_LOGI(TAG, "Chiavetta rimossa");
            }
            old_id = current_id;
            vTaskDelay(pdMS_TO_TICKS(500));
        }

        if (usb_connected && wait_for_event(EVENT_DEVICE_DISCONNECTED, 100)) {
            ESP_LOGI(TAG, "Device disconnected");
            xEventGroupClearBits(usb_flags, EVENT_READY_TO_UNINSTALL);
            ESP_ERROR_CHECK(msc_host_vfs_unregister(vfs_handle));
            ESP_ERROR_CHECK(msc_host_uninstall_device(msc_device));
            usb_connected = 0;

        } else if (wait_for_msc_device(&device_address, 100)) {
            ESP_LOGI(TAG, "New USB device: %i", device_address);
            if (msc_host_install_device(device_address, &msc_device) == ESP_OK) {

                // msc_host_print_descriptors(msc_device);
                ESP_ERROR_CHECK(msc_host_get_device_info(msc_device, &info));

                esp_err_t err = msc_host_vfs_register(msc_device, MSC_USB_MOUNTPOINT, &mount_config, &vfs_handle);
                if (err == ESP_OK) {
                    char **strings = NULL;
                    int    res     = archive_management_list_archives(MSC_USB_MOUNTPOINT, &strings);

                    xSemaphoreTake(sem, portMAX_DELAY);
                    utils_free_string_list(listed_archives, listed_archives_num);
                    listed_archives = strings;
                    if (res > 0) {
                        listed_archives_num = res;
                    } else {
                        listed_archives_num = 0;
                    }

                    for (size_t i = 0; i < listed_archives_num; i++) {
                        ESP_LOGI(TAG, "Archive %zu: %s", i, strings[i]);
                    }
                    xSemaphoreGive(sem);

                    xEventGroupSetBits(usb_flags, EVENT_DRIVE_READY);
                } else {
                    xEventGroupSetBits(usb_flags, EVENT_DRIVE_FAILED);
                    ESP_LOGW(TAG, "Unable to setup the vfs: 0x%X", err);
                }

                usb_connected = 1;
                ESP_LOGI(TAG, "USB device successfully connected and mounted");
            } else {
                xEventGroupSetBits(usb_flags, EVENT_DRIVE_FAILED);
                ESP_LOGW(TAG, "Unable to install msc device!");
            }
        }

        /*
        task_message_t message;
        if (xQueueReceive(message_queue, &message, pdMS_TO_TICKS(100))) {
            switch (message.code) {
                case TASK_MESSAGE_CODE_LOAD_ARCHIVE: {
                    char string[128] = {0};
                    snprintf(string, sizeof(string), "%s/%s%s", MSC_USB_MOUNTPOINT, message.archive_name,
        ARCHIVE_SUFFIX); msc_response_t response = {.code = MSC_RESPONSE_CODE_ARCHIVE_EXTRACTION_COMPLETE};
                    response.error          = archive_management_extract_configuration(string);
                    xQueueSend(response_queue, &response, portMAX_DELAY);
                    break;
                }

                case TASK_MESSAGE_CODE_SAVE_ARCHIVE: {
                    msc_response_t response = {.code = MSC_RESPONSE_CODE_ARCHIVE_SAVING_COMPLETE};
                    response.error          = archive_management_save_configuration(MSC_USB_MOUNTPOINT,
        message.archive_name); xQueueSend(response_queue, &response, portMAX_DELAY); break;
                }
            }
        }
        */
    }

    vTaskDelete(NULL);
}


static uint8_t wait_for_msc_device(uint8_t *address, unsigned long delay_ms) {
    EventBits_t event;

    event = xEventGroupWaitBits(usb_flags, EVENT_DEVICE_CONNECTED | EVENT_DEVICE_ADDRESS_MASK, pdTRUE, pdFALSE,
                                pdMS_TO_TICKS(delay_ms));
    if ((event & EVENT_DEVICE_CONNECTED) > 0) {
        // Extract USB device address from event group bits
        *address = (event & EVENT_DEVICE_ADDRESS_MASK) >> 4;
        return 1;
    } else {
        return 0;
    }
}


static bool wait_for_event(EventBits_t event, TickType_t timeout) {
    return xEventGroupWaitBits(usb_flags, event, pdTRUE, pdTRUE, timeout) & event;
}


static void utils_free_string_list(char **strings, size_t len) {
    for (size_t i = 0; i < len; i++) {
        free(strings[i]);
    }
    free(strings);
}
