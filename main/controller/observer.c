#include "watcher.h"
#include "model/model.h"


static watcher_t watcher = {0};


void observer_init(mut_model_t *model) {
    WATCHER_INIT_STD(&watcher, model);
}

