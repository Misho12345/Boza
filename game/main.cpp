#include "BozaEngine.hpp"

int main()
{
    const auto h = boza_create_app();

    if (!boza_app_init(h))
    {
        boza_destroy_app(h);
        return -1;
    }

    boza_app_run(h);
    boza_destroy_app(h);
}
