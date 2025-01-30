#ifndef FUP_H_INCLUDED
#define FUP_H_INCLUDED


#include <stdint.h>
#include "model/model.h"


firmware_update_state_t fup_get_state(void);
uint8_t                 fup_proceed(void);
void                    fup_manage(void);


#endif
