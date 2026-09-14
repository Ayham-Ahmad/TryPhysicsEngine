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
    std::fill(_cells.begin(), _cells.end(), 0);

    int rows = (Globals::sh + Globals::gridSize - 1) / Globals::gridSize;
    int cols = (Globals::sw + Globals::gridSize - 1) / Globals::gridSize;

    // Bucket each particle into the cells its bounding box overlaps
    for (const ParticleObject &p : particles)
    {
        int minRow = (p.rigidBody.position.y - p.radius) / Globals::gridSize;
        int maxRow = (p.rigidBody.position.y + p.radius) / Globals::gridSize;
        int minCol = (p.rigidBody.position.x - p.radius) / Globals::gridSize;
        int maxCol = (p.rigidBody.position.x + p.radius) / Globals::gridSize;

        if (minRow < 0) minRow = 0;
        if (maxRow >= rows) maxRow = rows - 1;
        if (minCol < 0) minCol = 0;
        if (maxCol >= cols) maxCol = cols - 1;

        for (int row = minRow; row <= maxRow; row++)
        {
            for (int col = minCol; col <= maxCol; col++)
            {
                _cells[row * cols + col]++;
            }
        }
    }
}

// Render the Grid
void Grid::render(SDL_Renderer *r, const bool isStarted) const
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

        float scale = std::min(1.0f, c / Globals::particlesCount);
        int value = static_cast<int>(255.0f * scale);
        SDL_SetRenderDrawColor(r, value, value, value, 255);

        // if (c == 0)
        //     SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        // else
        //     SDL_SetRenderDrawColor(r, 255, 255, 255, 255);

        SDL_RenderFillRect(r, &_grid[i]);
    }
}