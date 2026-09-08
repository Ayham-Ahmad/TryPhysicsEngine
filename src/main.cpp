#include "Game.h"

#include "Game.cpp"

double deltaTime = 0.0;

int main()
{
    Game game;
    game.init();

    game.NOW = SDL_GetPerformanceCounter();

    while (game.running)
    {
        SDL_GetMouseState(&game.mouseX, &game.mouseY);

        game.LAST = game.NOW;
        game.NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((game.NOW - game.LAST) / (double)SDL_GetPerformanceFrequency());

        game.handleInput();
        game.update();
        game.render();

        game.timer.sleep();
    }

    return 0;
}
