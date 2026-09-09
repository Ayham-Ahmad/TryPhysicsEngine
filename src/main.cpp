#include "game.h"

#include "game.cpp"
#include "panel.cpp"
#include "particle.cpp"
#include "collision.cpp"
#include "physics.cpp"

double deltaTime = 0.0;

int main()
{
    // Main object
    Game game;
    // Initialize the main window
    game.init();

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
        deltaTime = (double)((game.NOW - game.LAST) / (double)SDL_GetPerformanceFrequency());

        // Main functions
        game.handleInput();
        game.update();
        game.render();

        // Keep it at 60 FPS
        game.timer.sleep();
    }

    return 0;
}
