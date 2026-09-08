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

    // _screenWidth = 800;
    // _screenHeight = 600;

    // Create window and
    window = SDL_CreateWindow("Test", _screenWidth, _screenHeight, SDL_WINDOW_BORDERLESS);
    r = SDL_CreateRenderer(window, nullptr);

    for (int i = 0; i < _pc; i++) {
        Particle p;
        p.rad = 50;
        p.res = 3 + i;
        _particles.push_back(p);
    }

    Particle circle;
    circle.rad = 50;
    circle.res = 100;
    _particles.push_back(circle);
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
    int i = 0;
    for (Particle& p : _particles) {
        int spaceing = p.rad * 2;
        int c = _screenWidth/2.0f - spaceing * (_pc/2);
        p.pos.x = c + i * spaceing;
        p.pos.y = _screenHeight/2.0f;
        i++;
    }

    _updateFPS();
}

// --- render ---
void Game::render()
{
    _clear({0, 0, 0, 255});

    for (Particle& p : _particles) {
        p.s.circle(r, p.pos, p.rad, p.res);
        showXAndYPosition(r, p.pos.x, p.pos.y, -20);
    }

    // s.triangle(r, {500, 400}, {480, 550}, {520, 550}, _colors.BlueF());

    showXAndYPosition(r, mouseX, mouseY);

    _showFPS();

    _present();
}
