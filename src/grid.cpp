#include "grid.h"

// Initialize grid
void Grid::initialize()
{
    float GRID_SIZE = Globals::gridSize;

    int rows = (Globals::sh + GRID_SIZE - 1) / GRID_SIZE;
    int cols = (Globals::sw + GRID_SIZE - 1) / GRID_SIZE;

    _grid.clear();
    _cells.clear();

    for (int i = 0; i < rows; i++)
    {
        float y = i * GRID_SIZE;
        float height = std::min(GRID_SIZE, Globals::sh - y);

        for (int j = 0; j < cols; j++)
        {
            float x = j * GRID_SIZE;
            float width = std::min(GRID_SIZE, Globals::sw - x);

            SDL_FRect rect = {x, y, width, height};

            _grid.push_back(rect);
            _cells.push_back(0);
        }
    }
}

// Update gird
void Grid::update(const std::vector<ParticleObject> &particles)
{
    _cells.clear();

    for (const SDL_FRect &g : _grid)
    {
        int particlesCount = 0;

        for (const ParticleObject &p : particles)
        {
            if (
                p.rigidBody.position.x + p.radius > g.x &&
                p.rigidBody.position.x - p.radius < g.x + g.w &&

                p.rigidBody.position.y + p.radius > g.y &&
                p.rigidBody.position.y - p.radius < g.y + g.h)
            {
                particlesCount++;
            }
        }

        _cells.push_back(particlesCount);
    }
}

// Render the Grid
void Grid::render(SDL_Renderer *r, const bool isStarted)
{
    for (size_t i = 0; i < _grid.size(); i++)
    {
        if (!isStarted)
        {
            SDL_SetRenderDrawColor(r, 100, 255, 100, 255);
            SDL_RenderRect(r, &_grid[i]);
            continue;
        }

        int c = _cells[i];

        float scale = c / Globals::particlesCount * 100;
        SDL_SetRenderDrawColor(r, 255 * scale, 255 * scale, 255 * scale, 255);

        // if (c == 0)
        //     SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        // else
        //     SDL_SetRenderDrawColor(r, 255, 255, 255, 255);

        SDL_RenderFillRect(r, &_grid[i]);
    }
}