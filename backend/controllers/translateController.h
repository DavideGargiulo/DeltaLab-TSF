#pragma once
#include "mongoose.h"
#include <stdbool.h>

bool translate_controller(struct mg_connection* c, struct mg_http_message* hm);
