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

    // The sim runs in meters; the screen (pixels) is converted once here
    Globals::worldW = Globals::sw / Globals::pixelsPerMeter;
    Globals::worldH = Globals::sh / Globals::pixelsPerMeter;

    // Create window and renderer
    window = SDL_CreateWindow("Fluid", Globals::sw, Globals::sh, SDL_WINDOW_BORDERLESS);
    r = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    SDL_SetRenderVSync(r, 1);

    // Initialize the contol panel
    _panel.init(mode->w);

    // Initialize graph
    _graph.init();

    // Initialize particles and grid
    init();
}

// Update the simulation clock once per frame
void Game::tick()
{
    _lastTime = _nowTime;
    _nowTime = SDL_GetPerformanceCounter();
    Globals::deltaTime = static_cast<double>(_nowTime - _lastTime) / SDL_GetPerformanceFrequency();
}

// Destructor
Game::~Game()
{
    close();

    _graph.close();
    _panel.close();

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
    Globals::particleMass = _panel.value("mass");
    Globals::pressureMultiplier = _panel.value("pressure");

    const float gridPitch = Globals::particlesSpacing + Globals::particleSize * 2.0f;
    Globals::targetDensity = Globals::particleMass / (gridPitch * gridPitch);

    // Initialize particles
    _particles.init(r);

    // Initialize grid
    _grid.init();

    // updating graph limits
    _graph.updateLimits();
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

        const std::string text = "FPS: " + std::to_string(_displayedFPS);
        if (text != _fpsText)
            _fpsText = text;
    }
}

// --- Display the FPS on the screen ---
void Game::_showFPS() const
{
    drawText(r, _fpsText, 10, 10, 25, {255, 0, 0, 255});
}

// --- clear and present ---
void Game::_clear() const
{
    Colors::Black(r);
    SDL_RenderClear(r);
}

// --- handleInput ---
void Game::handleInput()
{
    while (SDL_PollEvent(&_e))
    {
        _panel.handleEvent(_e);
        _graph.handleEvent(_e);

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
}

void Game::_listener()
{
    Globals::grid = _panel.buttonValue("grid");
    Globals::graph = _panel.buttonValue("graph");
    Globals::isGravity = _panel.buttonValue("gravity");
    Globals::screenCollision = _panel.buttonValue("screenCollision");
    Globals::particles = _panel.buttonValue("particles");
    Globals::circles = _panel.buttonValue("circles");
    Globals::gradientArrows = _panel.buttonValue("gradientArrows");
    Globals::speedColoring = _panel.buttonValue("speedColoring");
    Globals::random = _panel.buttonValue("random");

    if (Globals::graph && !_graphWasEnabled)
    {
        _graph.init(Globals::sw, Globals::sh);
        _graph.updateLimits();
    }
    else if (!Globals::graph && _graphWasEnabled)
        _graph.close();

    _graphWasEnabled = Globals::graph;

    if (_panel.btnListener("reset"))
    {
        _accumulator = 0.0;
        init();
        return;
    }

    if (_panel.sliderListener("pressure"))
    {
        Globals::pressureMultiplier = _panel.value("pressure");
    }

    // Randomize the particles positions
    if (_panel.btnListener("random"))
    {
        _particles.init(r);
    }
}

void Game::_sliders_lestener()
{
    const bool spacingChanged = _panel.sliderListener("spacing");
    const bool sizeChanged = _panel.sliderListener("size");
    const bool massChanged = _panel.sliderListener("mass");

    // Update particle settings if changed
    if (_panel.sliderListener("count"))
    {
        Globals::particlesCount = _panel.value("count");
        _particles.init(r);
        _graph.updateLimits();
    }

    if (spacingChanged)
    {
        Globals::particlesSpacing = _panel.value("spacing");
        _particles.init(r);
    }

    if (sizeChanged)
    {
        Globals::particleSize = _panel.value("size");
        _particles.init(r);
    }

    // Update grid settings if changed
    if (_panel.sliderListener("gridSize"))
    {
        Globals::gridSize = _panel.value("gridSize");
        _grid.init();
    }

    // Update smoothing radius if changed
    if (_panel.sliderListener("smoothingRadius"))
    {
        Globals::smoothingRadius = _panel.value("smoothingRadius");
    }

    // Update particle mass if changed (applies live to the equations)
    if (massChanged)
    {
        Globals::particleMass = _panel.value("mass");
        _graph.updateLimits();
    }

    // Manual override for the target density used by pressure calculations
    if (_panel.sliderListener("targetDensity"))
    {
        Globals::targetDensity = _panel.value("targetDensity");
        _graph.updateLimits();
    }

    // Re-derive the rest density from the actual grid pitch after any layout
    // change; otherwise real density stays far below targetDensity and the
    // max(0,...) clamp in fluid.cpp pins every pressure force to zero.
    if (spacingChanged || sizeChanged || massChanged)
    {
        const float gridPitch = Globals::particlesSpacing + Globals::particleSize * 2.0f;
        Globals::targetDensity = Globals::particleMass / (gridPitch * gridPitch);
    }
}

// --- update ---
void Game::update()
{
    _listener();

    // Don't update if not started yet
    if (!_panel.buttonValue("start"))
    {
        _sliders_lestener();
        _accumulator = 0.0;
        _updateFPS();
        return;
    }

    // Don't update if paused
    if (_panel.buttonValue("pause"))
    {
        _updateFPS();
        return;
    }

    // Run the physics at a fixed rate regardless of the render speed
    constexpr double STEP_TIME = 1.0 / 120.0;

    _accumulator += Globals::deltaTime * _panel.value("speed");

    // Guard against the spiral of death, but keep enough headroom that the speed
    // slider still scales (cap of 10 steps/frame throttled high speeds at low FPS).
    if (_accumulator > STEP_TIME * 120.0)
        _accumulator = STEP_TIME * 120.0;

    Globals::gravity = _panel.value("gravity");

    // Update particles
    while (_accumulator >= STEP_TIME)
    {
        _particles.update(STEP_TIME);
        _accumulator -= STEP_TIME;
    }

    // Update grid
    if (Globals::grid)
        _grid.update(_particles.getParticlesList());

    // Update metrics for the graph and panel
    const PhysicsStats stats = _particles.getPhysicsStats();

    _panel.setText("entropy", stats.entropy);
    _panel.setText("speed", stats.speed);
    _panel.setText("kineticEnergy", stats.kineticEnergy);
    _panel.setText("acceleration", stats.acceleration);
    _panel.setText("density", stats.density);

    if (Globals::graph)
    {
        _graph.value("entropy", stats.entropy);
        _graph.value("speed", stats.speed);
        _graph.value("kineticEnergy", stats.kineticEnergy);
        _graph.value("acceleration", stats.acceleration);
        _graph.value("density", stats.density);
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
    _particles.render(r);

    // Render panel (capped)
    _panelTimer += Globals::deltaTime;
    if (_panelTimer >= PANEL_REFRESH_RATE)
    {
        float mouseX = 0.0f, mouseY = 0.0f;
        SDL_GetMouseState(&mouseX, &mouseY);
        _panelTimer -= PANEL_REFRESH_RATE;
        _panel.render(mouseX, mouseY);
    }

    // Render graph (capped)
    if (Globals::graph && _graph.isOpen())
    {
        _graphTimer += Globals::deltaTime;
        if (_graphTimer >= GRAPH_REFRESH_RATE)
        {
            _graphTimer -= GRAPH_REFRESH_RATE;
            _graph.render();
        }
    }

    // Render FPS counter
    _showFPS();

    // Render everything
    SDL_RenderPresent(r);
}
