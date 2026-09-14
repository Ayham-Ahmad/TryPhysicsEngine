#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <algorithm>

#include "particle.h"
#include "FPSTimer.h"
#include "text.h"
#include "physics.h"
#include "globals.h"
#include "colors.h"
#include "shapes.h"
#include "panel.h"
#include "collision.h"
#include "grid.h"
#include "graph.h"
#include "window.h"

class Game : public Window
{
public:
    Game();
    ~Game();

    void init();

    Timer<10000> timer;

public:
    // Core game loop functions
    void handleInput();
    void update();
    void render();

private:
    // Clear and present
    void _clear() const;
    void _present() const;

    // Event
    SDL_Event _e;

public:
    // Quit flag
    bool running = true;

    Uint64 NOW, LAST;
    float mouseX, mouseY;

    // --- Refresh rates ---
    static constexpr double GRAPH_REFRESH_RATE = 1.0 / 30.0;
    static constexpr double PANEL_REFRESH_RATE = 1.0 / 24.0;

    // --- FPS system ---
private:
    double _fpsTimer = 0.0;        // time accumulator for measuring FPS
    int _fpsCounter = 0;           // counts frames in one second
    int _currentFPS = 0;           // calculated FPS
    double _fpsDisplayTimer = 0.0; // timer for updating displayed FPS
    int _displayedFPS = 0;         // stable FPS value shown on screen

    void _updateFPS();
    void _showFPS() const;
    void _listener();

    // --- Vars ---
private:
    Shape s;
    Colors _colors;
    Panel _panel;
    Particle _particles;
    Grid _grid;
    Graph _graph;

    // --- Timers ---
    double _panelTimer = 0.0;
    double _graphTimer = 0.0;
    double _accumulator = 0.0;
    bool _graphWasEnabled = false;
};