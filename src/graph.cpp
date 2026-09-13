#include "graph.h"

void Graph::init()
{
    _values = {
        {"entropy", 0.0f, 1.0f},
        {"speed", 0.0f, 200.0f},
        {"kineticEnergy", 0.0f, 1000000.0f},
        {"acceleration", 0.0f, 2000.0f},
    };
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

void Graph::render(SDL_Renderer *r)
{
    SDL_FRect graphBgRect = {GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT};

    _c.VeryDarkGray(r);
    SDL_RenderFillRect(r, &graphBgRect);

    for (size_t i = 0; i < _values.size(); i++)
    {
        const GraphValue &graphValue = _values[i];

        SDL_SetRenderDrawColor(r, _colors[i].r, _colors[i].g, _colors[i].b, _colors[i].a);

        std::queue<float> temp = graphValue.values;

        float x = GRAPH_X;
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

            float y = GRAPH_Y + (1.0f - normalized) * (GRAPH_HEIGHT - pointSize);

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
}