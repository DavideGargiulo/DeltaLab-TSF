#pragma once
#include "mongoose.h"
#include <stdbool.h>

bool user_controller(struct mg_connection* c, struct mg_http_message* hm);
