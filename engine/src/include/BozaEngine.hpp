#pragma once
#include "boza/API.hpp"

BOZA_C_API void* boza_create_app();
BOZA_C_API bool boza_app_init(void* h);
BOZA_C_API void boza_app_run(void* h);
BOZA_C_API void boza_destroy_app(void* h);
