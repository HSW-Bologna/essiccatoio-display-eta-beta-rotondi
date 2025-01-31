#include "bsp/msc.h"


int msc_get_response(msc_response_t *response) {
    (void)response;
    return 0;
}


removable_drive_state_t msc_is_device_mounted(void) {
    return REMOVABLE_DRIVE_STATE_MISSING;
}
