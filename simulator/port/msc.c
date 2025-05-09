#include "bsp/msc.h"


int msc_get_response(msc_response_t *response) {
    (void)response;
    return 0;
}


removable_drive_state_t msc_is_device_mounted(void) {
    return REMOVABLE_DRIVE_STATE_MOUNTED;
}


void msc_enable_usb(uint8_t enabled) {
    (void)enabled;
}


size_t msc_read_archives(mut_model_t *pmodel) {
    (void)pmodel;
    return 0;
}


void msc_extract_archive(const char *archive) {
    (void)archive;
}


void msc_save_archive(const char *archive) {
    (void)archive;
}
