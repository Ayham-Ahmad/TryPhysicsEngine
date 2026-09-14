#include "game.h"

int main()
{
    // Initialize the main window
    Game game;

    // Initialize timer
    game.NOW = SDL_GetPerformanceCounter();

    // Main loop
    while (game.running)
    {
        // Get mouse x & y position
        SDL_GetMouseState(&game.mouseX, &game.mouseY);

        // Update the timer
        game.LAST = game.NOW;
        game.NOW = SDL_GetPerformanceCounter();
        Globals::deltaTime = (double)((game.NOW - game.LAST) / (double)SDL_GetPerformanceFrequency());

        // Main functions
        game.handleInput();
        game.update();
        game.render();

        // Keep it at N FPS
        game.timer.sleep();
    }

    return 0;
}
