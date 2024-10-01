#ifndef CONTROLLER_H_INCLUDED
#define CONTROLLER_H_INCLUDED

#include "model/model.h"
#include "adapters/view/view.h"


void controller_init(mut_model_t *p_model);
void controller_manage(mut_model_t *p_model);
void controller_process_message(pman_handle_t handle, void *msg);
void controller_sync_minion(model_t *model);


#endif
