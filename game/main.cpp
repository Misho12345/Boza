import game;

int main()
{
    game::MaterialShowcase app;
    if (!app.init()) return -1;
    app.run();
}
