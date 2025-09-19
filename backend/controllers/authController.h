#pragma once
#include "mongoose.h"
#include <stdbool.h>

bool auth_controller(struct mg_connection* c, struct mg_http_message* hm);
