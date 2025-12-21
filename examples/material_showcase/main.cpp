import material_showcase;

int main()
{
    MaterialShowcase app;
    if (!app.init()) return -1;
    app.run();
}
