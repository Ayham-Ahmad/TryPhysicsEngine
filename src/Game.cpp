#include "Game.h"

// Constructor / Destructor
Game::Game()
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
}

Game::~Game()
{
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void Game::init()
{
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(displayID);

    _screenWidth = mode->w;
    _screenHeight = mode->h;

    // Create window and
    window = SDL_CreateWindow("Test", _screenWidth, _screenHeight, SDL_WINDOW_FULLSCREEN);
    r = SDL_CreateRenderer(window, nullptr);

    // for (int i = 0; i < 10; i++) {
    //     Particle p;
    //     p.obj.affectedByGravity = true;
    //     p.rad = 10;
    //     p.obj.x0 = _screenWidth/2 + ((p.rad + 20)  * 2 * i);
    //     p.obj.y0 = _screenHeight/2;
    //     particles.push_back(p);
    // }
}

// --- FPS Handling ---
void Game::_updateFPS()
{
    _fpsCounter++;
    _fpsTimer += deltaTime;
    _fpsDisplayTimer += deltaTime;

    // Calculate FPS once per second
    if (_fpsTimer >= 1.0)
    {
        _currentFPS = static_cast<int>(_fpsCounter / _fpsTimer);
        _fpsCounter = 0;
        _fpsTimer = 0.0;
    }

    // Update displayed FPS once per second (for smooth text)
    if (_fpsDisplayTimer >= 1.0)
    {
        _displayedFPS = _currentFPS;
        _fpsDisplayTimer = 0.0;
    }
}

void Game::_showFPS()
{
    drawText(r, "FPS: " + std::to_string(_displayedFPS), 10, 10, 25);
}

// --- clear and present ---
void Game::_clear(SDL_Color color)
{
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_RenderClear(r);
}

void Game::_present()
{
    SDL_RenderPresent(r);
}

// --- handleInput ---
void Game::handleInput()
{
    while (SDL_PollEvent(&_e))
    {
        // Close window
        if (_e.type == SDL_EVENT_QUIT)
            running = false;
        else if (_e.type == SDL_EVENT_KEY_DOWN)
        {
            SDL_Scancode sc = _e.key.scancode;

            // Close window
            if (sc == SDL_SCANCODE_ESCAPE)
                running = false;
        }
    }

    const bool *state = SDL_GetKeyboardState(NULL); // For continuos pressing
}

// --- update ---
void Game::update()
{
    // for (Particle& p : particles) {
    //     p.obj.x = _applyPhysics.getFinalPosition(0, p.obj.vx0, p.obj.x0, p.obj.totalTime);
    //     p.obj.y = _applyPhysics.getFinalPosition(GRAVITY, p.obj.vy0, p.obj.y0, p.obj.totalTime);

    //     std::cout << p.obj.vx0 << " "
    //               << p.obj.vy0 << " "
    //               << p.obj.x0 << " "
    //               << p.obj.y0 << " "
    //               << p.obj.y << " "
    //               << p.obj.x << " "
    //               << p.obj.totalTime << "\n";
    // }

    _updateFPS();
}

// --- render ---
void Game::render()
{
    _clear({0, 0, 0, 255});

    // SDL_SetRenderDrawColor(r, 100, 255, 90, 255);
    // for (Particle& p : particles) {
    //     p.Circle.render(r, p.obj.x, p.obj.y, p.rad);
    //     showXAndYPosition(r, p.obj.x, p.obj.y, -20);
    // }

    showXAndYPosition(r, mouseX, mouseY);

    _showFPS();

    _present();
}
