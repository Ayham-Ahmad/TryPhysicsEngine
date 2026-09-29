#include "graph.h"

Graph::~Graph()
{
    close();
}

void Graph::init()
{
    _values = {
        {"entropy", 0.0f, 1.0f},
        {"speed", 0.0f, 200.0f},
        {"kineticEnergy", 0.0f, 0.5f * Globals::particleMass * Globals::velocityMax * Globals::velocityMax},
        {"acceleration", 0.0f, 2000.0f},
        {"density", 0.0f, 2.0f * Globals::targetDensity},
    };

    // Pre-allocate the ring storage once (fixed history length)
    const size_t capacity = static_cast<size_t>(MAX_SAMPLES);
    for (GraphValue &gv : _values)
        gv.values.resize(capacity);

    updateLimits();
}

void Graph::updateLimits()
{
    for (GraphValue &gv : _values)
    {
        if (gv.id == "kineticEnergy")
            gv.max = 0.5f * Globals::particleMass * Globals::velocityMax * Globals::velocityMax;
        else if (gv.id == "density")
            gv.max = 2.0f * Globals::targetDensity;
    }
}

void Graph::init(const float sw, const float sh)
{
    init();

    // Create the graph window
    window = SDL_CreateWindow(
        "Graph",
        GRAPH_WIDTH + WINDOW_MARGIN,
        GRAPH_HEIGHT + WINDOW_MARGIN * 2,
        SDL_WINDOW_RESIZABLE);

    // Create the renderer for graph
    r = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Get window ID
    windowID = SDL_GetWindowID(window);

    // Place the window at the top-left under the FPS counter
    SDL_SetWindowPosition(window, 0, 80);
}

void Graph::handleEvent(const SDL_Event &event)
{
    // Ignore events if the graph is closed or the event is from another window
    if (!window || eventWindowID(event) != windowID)
        return;

    // Close the graph if the close button is pressed
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
        close();
}

void Graph::value(const std::string &id, float value)
{
    const size_t capacity = static_cast<size_t>(MAX_SAMPLES);

    for (GraphValue &graphValue : _values)
    {
        if (graphValue.id == id)
        {
            // Append while there is room; once full, overwrite the oldest sample
            if (graphValue.size < capacity)
                graphValue.size++;
            else
                graphValue.head = (graphValue.head + 1) % capacity;

            graphValue.values[(graphValue.head + graphValue.size - 1) % capacity] = value;

            return;
        }
    }
}

void Graph::render() const
{
    if (!window || !r)
        return;

    // Clear the graph
    Colors::VeryDarkGray(r);
    SDL_RenderClear(r);

    for (size_t i = 0; i < _values.size(); i++)
    {
        const GraphValue &graphValue = _values[i];

        SDL_SetRenderDrawColor(r, _colors[i].r, _colors[i].g, _colors[i].b, _colors[i].a);

        const size_t capacity = static_cast<size_t>(MAX_SAMPLES);

        float x = 0.0f;
        float pointSize;
        int index = 0;
        size_t lastIndex = graphValue.size - 1;

        // Walk the ring in chronological order without copying the buffer
        for (size_t k = 0; k < graphValue.size; k++)
        {
            if (index == lastIndex)
                pointSize = POINT_SIZE * 3;
            else
                pointSize = POINT_SIZE;

            const float value = graphValue.values[(graphValue.head + k) % capacity];

            float normalized = (value - graphValue.min) / (graphValue.max - graphValue.min);
            normalized = std::clamp(normalized, 0.0f, 1.0f);

            float y = (1.0f - normalized) * (GRAPH_HEIGHT - pointSize);

            SDL_FRect point = {x, y, pointSize, pointSize};
            SDL_RenderFillRect(r, &point);

            if (index == lastIndex)
            {
                std::ostringstream text;
                text << std::fixed << std::setprecision(2) << value;
                drawText(r, graphValue.id + " " + text.str(), x, y + 3, 16);
            }

            x += POINT_SPACING;
            index++;
        }
    }

    // Show the rendered graph
    SDL_RenderPresent(r);
}