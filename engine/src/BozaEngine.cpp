#include "include/BozaEngine.hpp"
#include "App.hpp"

BOZA_C_API void* boza_create_app() { return new boza::App(); }
BOZA_C_API bool boza_app_init(void* h) { return h ? static_cast<boza::App*>(h)->init() : false; }
BOZA_C_API void boza_app_run(void* h) { if (h) static_cast<boza::App*>(h)->run(); }
BOZA_C_API void boza_destroy_app(void* h) { if (h) delete static_cast<boza::App*>(h); }
