#ifndef MSC_H_INCLUDED
#define MSC_H_INCLUDED


#include <stdint.h>
#include "model/model.h"


#define MSC_USB_MOUNTPOINT "/usb"


typedef enum {
    MSC_RESPONSE_CODE_ARCHIVE_EXTRACTION_COMPLETE,
    MSC_RESPONSE_CODE_ARCHIVE_SAVING_COMPLETE,
} msc_response_code_t;


typedef struct {
    msc_response_code_t code;
    union {
        int error;
    };
} msc_response_t;


void                    msc_init(void);
size_t                  msc_read_archives(model_t *pmodel);
removable_drive_state_t msc_is_device_mounted(void);
void                    msc_extract_archive(const name_t archive);
void                    msc_save_archive(const name_t archive);
int                     msc_get_response(msc_response_t *response);


#endif
