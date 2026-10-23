#include "game.h"

int main()
{
    // Initialize the main window
    Game game;

    // Main loop
    while (game.running)
    {
        

        game.tick();
        game.handleInput();
        game.update();
        game.render();
    }

    return 0;
}