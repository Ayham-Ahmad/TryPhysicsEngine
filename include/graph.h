#pragma once

#include "SDL3/SDL.h"

#include <queue>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "colors.h"
#include "text.h"
#include "window.h"

struct GraphValue
{
    std::string id;
    float min;
    float max;
    std::queue<float> values;
};

class Graph : public Window
{
private:
    static constexpr float GRAPH_WIDTH = 200.0f * 3.0f;
    static constexpr float GRAPH_HEIGHT = 100.0f * 3.0f;
    static constexpr float WINDOW_MARGIN = 20.0f;

    static constexpr int MAX_SAMPLES = 160;
    static constexpr float POINT_SIZE = 3.0f;
    static constexpr float POINT_SPACING = 3.0f;

    SDL_Color _colors[7] = {
        {255, 0, 0, 150},     // Red
        {0, 255, 0, 150},     // Green
        {0, 0, 255, 150},     // Blue
        {255, 255, 0, 150},   // Yellow
        {255, 0, 255, 150},   // Magenta
        {0, 255, 255, 150},   // Cyan
        {255, 128, 0, 150}    // Orange
    };

    std::vector<GraphValue> _values;
    Colors _c;

public:
    ~Graph();

    void init();
    void init(float sw, float sh);

    void handleEvent(const SDL_Event &event);
    void value(const std::string &id, float value);
    void render() const;
};