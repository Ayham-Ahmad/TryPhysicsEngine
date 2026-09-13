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
    // Globals::sw = 800;
    // Globals::sh = 600;

    // Create window and renderer
    window = SDL_CreateWindow("Test", Globals::sw, Globals::sh, SDL_WINDOW_BORDERLESS);
    r = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Initialize the contorl panel
    _panel.init(mode->w);

    // Initialize graph
    _graph.init();

    init();
}

// Destructor
Game::~Game()
{
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void Game::init()
{
    Globals::particlesCount = _panel.value("count");
    Globals::particlesSpacing = _panel.value("spacing");
    Globals::particleSize = _panel.value("size");
    Globals::gridSize = _panel.value("gridSize");
    Globals::smoothingRadius = _panel.value("smoothingRadius");

    // Initialize particles
    _particles.init(r);

    // Initialize grid
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

void Game::_listener()
{
    Globals::grid = _panel.buttonValue("grid");
    Globals::graph = _panel.buttonValue("graph");
    Globals::isGravity = _panel.buttonValue("gravity");
    Globals::GForce = _panel.buttonValue("gforce");
    Globals::screenCollision = _panel.buttonValue("screenCollision");
    Globals::particles = _panel.buttonValue("particles");
    Globals::random = _panel.buttonValue("random");

    if (_panel.btnListener("reset"))
    {
        init();
        return;
    }

    // Update particle settings if changed
    if (_panel.sliderListener("count"))
    {
        Globals::particlesCount = _panel.value("count");
        _particles.init(r);
    }

    if (_panel.sliderListener("spacing"))
    {
        Globals::particlesSpacing = _panel.value("spacing");
        _particles.init(r);
    }

    if (_panel.sliderListener("size"))
    {
        Globals::particleSize = _panel.value("size");
        _particles.init(r);
    }

    // Update grid settings if changed
    if (_panel.sliderListener("gridSize"))
    {
        Globals::gridSize = _panel.value("gridSize");
        _grid.initialize();
    }

    // Update smoothing radius if changed
    if (_panel.sliderListener("smoothingRadius"))
    {
        Globals::smoothingRadius = _panel.value("smoothingRadius");
    }

    // Randomize the particles positions
    if (_panel.btnListener("random"))
    {
        _particles.init(r);
    }
}

// --- update ---
void Game::update()
{
    _listener();

    if (!_panel.buttonValue("start"))
    {
        _updateFPS();
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
    _particles.update(simulationDeltaTime);

    // Update grid
    if (Globals::grid)
        _grid.update(_particles.getParticlesList());

    // Update graph
    if (Globals::graph)
    {
        _graph.value("entropy", _physics2d.calculateEntropy(_particles.getParticlesList()));
        std::array<float, 3> values = _particles.getPhysicsValues();
        _graph.value("speed", values[0]);
        _graph.value("kineticEnergy", values[1]);
        _graph.value("acceleration", values[2]);
    }

    // Update FPS
    _updateFPS();
}

// --- render ---
void Game::render()
{
    // Clear background
    _clear();

    // Render Grid
    if (Globals::grid)
        _grid.render(r, _panel.buttonValue("start"));

    // Render particles
    if (Globals::particles)
        _particles.render(r);

    // Render panel
    _panel.render(mouseX, mouseY);

    // Render graph
    if (Globals::graph)
        _graph.render(r);

    // Render FPS counter
    _showFPS();

    // Render everything
    _present();
}
