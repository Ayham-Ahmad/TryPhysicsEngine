#include "game.h"
#include <algorithm>
#include <cmath>

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

    SDL_SetRenderVSync(r, 0);

    // Initialize the contol panel
    _panel.init(mode->w, window);

    // Initialize graph
    // _graph.init();

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
    Perf::Get().EndFrame();
    Perf::Get().ReportAndClear();
    close();

    // _graph.close();
    _panel.close();

    TTF_Quit();
    SDL_Quit();
}

void Game::init()
{
    const int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(Globals::simCount))));
    const int rows = (Globals::simCount + columns - 1) / columns;
    const int screenWidth = static_cast<int>(Globals::sw);
    const int screenHeight = static_cast<int>(Globals::sh);

    for (int i = 0; i < Globals::simCount; i++)
    {
        if (Globals::simCount == 1)
        {
            constexpr int boxWidth = 800;
            constexpr int boxHeight = 600;
            _sims[i].box = {
                (screenWidth - boxWidth) / 2,
                (screenHeight - boxHeight) / 2,
                boxWidth,
                boxHeight};
        }
        else
        {
            const int column = i % columns;
            const int row = i / columns;
            const int left = column * screenWidth / columns;
            const int right = (column + 1) * screenWidth / columns;
            const int top = row * screenHeight / rows;
            const int bottom = (row + 1) * screenHeight / rows;
            _sims[i].box = {left, top, right - left, bottom - top};
        }

        _applySimulationSettings(i);
        const auto &box = _sims[i].box;
        _sims[i].particles.init(r, box);
        _grid.init(box);
    }

    // Initialize grid
    // _applySimulationSettings(0);
    // _grid.init();

    // updating graph limits
    // _graph.updateLimits();

    // Initialize emitters
    _emitters.init();

    _updateParticleCountTexts();
}

void Game::_applySimulationSettings(int simulationIndex)
{
    Globals::gravity = _panel.value("gravity", simulationIndex);
    Globals::particlesCount = _panel.value("count", simulationIndex);
    Globals::particlesSpacing = _panel.value("spacing", simulationIndex);
    Globals::particleSize = _panel.value("size", simulationIndex);
    Globals::interatctionRadius = Globals::particleSize * 4.0f;
    Globals::targetDensity = _panel.value("target_density", simulationIndex);
    Globals::kNear = _panel.value("kNear", simulationIndex);
    Globals::k = _panel.value("k", simulationIndex);
    Globals::Sigma = _panel.value("sigma", simulationIndex);
    Globals::Beta = _panel.value("beta", simulationIndex);
    const float particleRadiusPixels = Globals::particleSize * Globals::pixelsPerMeter;
    Globals::PARTICLE_RESOLUTION = std::clamp(
        static_cast<int>(std::ceil(particleRadiusPixels * 2.0f)),
        3,
        32);
}

void Game::_updateParticleCountTexts()
{
    const int activeSimulation = _panel.activeSimulation();
    const int activeCount = static_cast<int>(_sims[activeSimulation].particles.getParticlesList().size());
    int totalCount = 0;
    for (const auto &sim : _sims)
        totalCount += static_cast<int>(sim.particles.getParticlesList().size());

    _panel.setText("count", static_cast<float>(activeCount));
    _panel.setText("totalCount", static_cast<float>(totalCount));
    _panel.setText("res", Globals::PARTICLE_RESOLUTION);
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
    PERF_SCOPE("Game::handleInput");
    while (SDL_PollEvent(&_e))
    {
        const bool wasEditing = _panel.isEditing();

        _panel.handleEvent(_e);
        // _graph.handleEvent(_e);

        // Close window
        if (_e.type == SDL_EVENT_QUIT)
            running = false;
        else if (_e.type == SDL_EVENT_KEY_DOWN)
        {
            SDL_Scancode sc = _e.key.scancode;

            // Escape cancels a value edit instead of closing the window
            if (sc == SDL_SCANCODE_ESCAPE && !wasEditing)
                running = false;
        }
        else if (_e.type == SDL_EVENT_MOUSE_MOTION)
        {
            mousePos.x = _e.motion.x / Globals::pixelsPerMeter;
            mousePos.y = _e.motion.y / Globals::pixelsPerMeter;
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
    Globals::emitters = _panel.buttonValue("emitters");
    Globals::random = _panel.buttonValue("random");

    // if (Globals::graph && !_graphWasEnabled)
    // {
    //     _graph.init(Globals::sw, Globals::sh);
    //     _graph.updateLimits();
    // }
    // else if (!Globals::graph && _graphWasEnabled)
    //     _graph.close();

    // _graphWasEnabled = Globals::graph;

    if (_panel.btnListener("reset"))
    {
        _accumulator = 0.0;
        init();
        return;
    }

    // if (_panel.sliderListener("pressure"))
    // {
    //     Globals::pressureMultiplier = _panel.value("pressure");
    // }

    // Emitter controls handled via globals in ParticleEmitter

    _applySimulationSettings(_panel.activeSimulation());

    // Randomize the particles positions
    if (_panel.btnListener("random"))
    {
        for (int i = 0; i < Globals::simCount; i++)
        {
            _applySimulationSettings(i);
            _sims[i].particles.init(r, _sims[i].box);
        }
    }
}

void Game::_sliders_lestener()
{
    const bool countChanged = _panel.sliderListener("count");
    const bool spacingChanged = _panel.sliderListener("spacing");
    const bool sizeChanged = _panel.sliderListener("size");
    _panel.sliderListener("target_density");

    if (countChanged || spacingChanged || sizeChanged)
    {
        for (int i = 0; i < Globals::simCount; i++)
        {
            _applySimulationSettings(i);
            _sims[i].particles.init(r, _sims[i].box);
        }

        const float particleRadiusPixels = Globals::particleSize * Globals::pixelsPerMeter;
        Globals::PARTICLE_RESOLUTION = std::clamp(
            static_cast<int>(std::ceil(particleRadiusPixels * 2.0f)),
            3,
            32);
    }
}

// --- update ---
void Game::update()
{
    static bool first = true;
    if (first)
    {
        Perf::Get().BeginFrame();
        first = false;
    }
    _listener();

    const bool started = _panel.buttonValue("start");
    _sliders_lestener();
    if (!started)
        _accumulator = 0.0;
    _updateParticleCountTexts();

    // Don't update if paused
    if (_panel.buttonValue("pause"))
    {
        _updateFPS();
        return;
    }

    // Run the physics at a fixed rate regardless of the render speed
    constexpr double STEP_TIME = 1.0 / 120.0;

    if (started)
        _accumulator += Globals::deltaTime * _panel.value("speed");

    // Update particles
    while (started && _accumulator >= STEP_TIME)
    {
        if (Globals::emitters)
        {
            PERF_SCOPE("Emitter::update");
            // _emitters.update(STEP_TIME, _particles.getParticlesList(), 0.01f);
        }
        {
            PERF_SCOPE("Particle::update");
            for (int i = 0; i < Globals::simCount; i++)
            {
                _applySimulationSettings(i);
                _sims[i].particles.update(STEP_TIME, mousePos);
            }
        }
        _accumulator -= STEP_TIME;
    }

    // Update grid
    if (Globals::grid)
    {
        PERF_SCOPE("Grid::update");
        _applySimulationSettings(0);
        _grid.update(_sims[0].particles.getParticlesList(), _sims[0].box);
    }

    // Update FPS
    _updateFPS();
}

// --- render ---
void Game::render()
{
    PERF_SCOPE("Game::render");
    // Clear background
    _clear();

    // Render emitters
    if (Globals::emitters)
        _emitters.render(r);

    // Render Grid
    if (Globals::grid)
    {
        _applySimulationSettings(0);
        _grid.render(r, _panel.buttonValue("start"));
    }

    // Render particles
    for (int i = 0; i < Globals::simCount; i++)
    {
        _applySimulationSettings(i);
        _sims[i].particles.render(r);
    }

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
    // if (Globals::graph && _graph.isOpen())
    // {
    //     _graphTimer += Globals::deltaTime;
    //     if (_graphTimer >= GRAPH_REFRESH_RATE)
    //     {
    //         _graphTimer -= GRAPH_REFRESH_RATE;
    //         _graph.render();
    //     }
    // }

    // Render FPS counter
    _showFPS();

    // Render everything
    {
        PERF_SCOPE("SDL_RenderPresent");
        SDL_RenderPresent(r);
    }
}
