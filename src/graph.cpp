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
        {"kineticEnergy", 0.0f, 10000000000.0f / Globals::particlesCount},
        {"acceleration", 0.0f, 2000.0f},
        {"density", 0.0f, 2000.0f},
    };
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
    for (GraphValue &graphValue : _values)
    {
        if (graphValue.id == id)
        {
            graphValue.values.push(value);

            if (graphValue.values.size() > MAX_SAMPLES)
                graphValue.values.pop();

            return;
        }
    }
}

void Graph::render() const
{
    if (!window || !r)
        return;

    // Clear the graph
    _c.VeryDarkGray(r);
    SDL_RenderClear(r);

    for (size_t i = 0; i < _values.size(); i++)
    {
        const GraphValue &graphValue = _values[i];

        SDL_SetRenderDrawColor(r, _colors[i].r, _colors[i].g, _colors[i].b, _colors[i].a);

        std::queue<float> temp = graphValue.values;

        float x = 0.0f;
        float pointSize;
        int index = 0;
        int lastIndex = temp.size() - 1;

        while (!temp.empty())
        {
            if (index == lastIndex)
                pointSize = POINT_SIZE * 3;
            else
                pointSize = POINT_SIZE;

            float normalized = (temp.front() - graphValue.min) / (graphValue.max - graphValue.min);
            normalized = std::clamp(normalized, 0.0f, 1.0f);

            float y = (1.0f - normalized) * (GRAPH_HEIGHT - pointSize);

            SDL_FRect point = {x, y, pointSize, pointSize};
            SDL_RenderFillRect(r, &point);

            if (index == lastIndex)
            {
                std::ostringstream text;
                text << std::fixed << std::setprecision(2) << temp.front();
                drawText(r, graphValue.id + " " + text.str(), x, y + 3, 16);
            }

            temp.pop();
            x += POINT_SPACING;
            index++;
        }
    }

    // Show the rendered graph
    SDL_RenderPresent(r);
}