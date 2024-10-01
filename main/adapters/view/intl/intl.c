#include <assert.h>
#include "intl.h"


const char *view_intl_get_string_in_language(uint16_t language, strings_t id) {
    return strings[id][language];
}


const char *view_intl_get_string(const model_t *model, strings_t id) {
    return view_intl_get_string_in_language(model->config.parmac.language, id);
}
