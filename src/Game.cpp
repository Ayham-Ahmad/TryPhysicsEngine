#include "game.h"

// Constructor
Game::Game()
{
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    // Get Display ID and mode
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(displayID);

    // Get screen dimansions
    Globals::sw = mode->w;
    Globals::sh = mode->h;
    // _screenWidth = 800;
    // _screenHeight = 600;

    // Create window and renderer
    window = SDL_CreateWindow("Test", Globals::sw, Globals::sh, SDL_WINDOW_BORDERLESS);
    r = SDL_CreateRenderer(window, nullptr);

    // Initialze the contorl panel
    _panel.init(mode->w);
}

// Destructor
Game::~Game()
{
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

// Initialize
void Game::init()
{
    Globals::particlesCount = _panel.value("count");
    Globals::particlesSpacing = _panel.value("spacing");
    Globals::particleSize = _panel.value("size");
    Globals::gridSize = _panel.value("gridSize");

    // Initialze particles
    _particles.initParticles();

    // Initialze grid
    _grid.initialize();
}

// --- FPS Handling ---
void Game::_updateFPS()
{
    _fpsCounter++;
    _fpsTimer += Globals::deltaTime;
    _fpsDisplayTimer += Globals::deltaTime;

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

// --- Display the FPS on the screen ---
void Game::_showFPS()
{
    drawText(r, "FPS: " + std::to_string(_displayedFPS), 10, 10, 25);
}

// --- clear and present ---
void Game::_clear()
{
    _colors.Black(r);
    SDL_RenderClear(r);
}

// --- Show to the screen ---
void Game::_present()
{
    SDL_RenderPresent(r);
}

// --- handleInput ---
void Game::handleInput()
{
    while (SDL_PollEvent(&_e))
    {
        _panel.handleEvent(_e);

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
    Globals::grid = _panel.buttonValue("grid");

    if (!_panel.buttonValue("start"))
    {
        _updateFPS();
        init();
        return;
    }

    // Don't update if paused
    if (_panel.buttonValue("pause"))
    {
        _updateFPS();
        return;
    }

    // Getting panel values
    const double simulationDeltaTime = Globals::deltaTime * _panel.value("speed");
    Globals::gravity = _panel.value("gravity");

    // Update particles
    _particles.updateParticles(simulationDeltaTime);

    // Update grid
    if (Globals::grid)
        _grid.update(_particles.getParticlesList());

    // Update FPS
    _updateFPS();
}

// --- render ---
void Game::render()
{
    // Clear the background
    _clear();

    // Render the Grid
    if (Globals::grid)
        _grid.render(r, _panel.buttonValue("start"));

    // Render the particles
    _particles.renderParticles(r);

    // Render the panel
    _panel.render(mouseX, mouseY);

    // Render the FPS counter
    _showFPS();

    // Render everything
    _present();
}
