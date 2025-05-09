#include "watcher.h"
#include "model/model.h"
#include "configuration/configuration.h"
#include "services/timestamp.h"
#include "bsp/msc.h"


static void password_cb(void *old_value, const void *new_value, watcher_size_t size, void *user_ptr, void *arg);
static void usb_cb(void *old_value, const void *new_value, watcher_size_t size, void *user_ptr, void *arg);


static watcher_t watcher = {0};


void observer_init(mut_model_t *model) {
    WATCHER_INIT_STD(&watcher, model);

    WATCHER_ADD_ARRAY_ENTRY_DELAYED(&watcher, model->config.password, STRING_NAME_SIZE, password_cb, NULL, 5000);
    WATCHER_ADD_ENTRY(&watcher, &model->run.removable_drive_enabled, usb_cb, NULL);
}


void observer_manage(mut_model_t *model) {
    (void)model;
    watcher_watch(&watcher, timestamp_get());
}


static void password_cb(void *old_value, const void *new_value, watcher_size_t size, void *user_ptr, void *arg) {
    (void)old_value;
    (void)new_value;
    (void)size;
    (void)arg;

    model_t *model = user_ptr;
    configuration_save_password(model->config.password);
}


static void usb_cb(void *old_value, const void *new_value, watcher_size_t size, void *user_ptr, void *arg) {
    (void)old_value;
    (void)new_value;
    (void)size;
    (void)arg;

    model_t *model = user_ptr;
    msc_enable_usb(model->run.removable_drive_enabled);
}
