#include "grid.h"

// Initialize grid
void Grid::init()
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

    const float ppu = Globals::pixelsPerMeter;

    const float GRID_SIZE = Globals::gridSize;

    int rows = (Globals::sh + GRID_SIZE - 1) / GRID_SIZE;
    int cols = (Globals::sw + GRID_SIZE - 1) / GRID_SIZE;

    for (size_t index = 0; index < particles.size(); index++)
    {
        const ParticleObject &p = particles[index];

        const float px = p.position.x * ppu;
        const float py = p.position.y * ppu;
        const float pSize = Globals::particleSize * ppu;

        int minRow = (py - pSize) / GRID_SIZE;
        int maxRow = (py + pSize) / GRID_SIZE;
        int minCol = (px - pSize) / GRID_SIZE;
        int maxCol = (px + pSize) / GRID_SIZE;

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

// Render Grid
void Grid::render(SDL_Renderer *r, const bool isStarted) const
{
    std::vector<SDL_FRect> batch;
    int lastColor = -1;
    const size_t size = batch.size();

    // TODO: instead of mulitple cells mearge them
    auto flush = [&]()
    {
        if (batch.empty())
            return;

        if (isStarted)
            SDL_RenderFillRects(r, batch.data(), size);
        else
            SDL_RenderRects(r, batch.data(), size);

        batch.clear();
    };

    if (!isStarted)
    {
        SDL_SetRenderDrawColor(r, 100, 255, 100, 255);
        SDL_RenderRects(r, _grid.data(), static_cast<int>(_grid.size()));
        return;
    }

    for (size_t i = 0; i < _grid.size(); i++)
    {
        const float scale = std::min(1.0f, static_cast<float>(_cells[i]) / Globals::particlesCount);
        const int color = static_cast<int>(255.0f * scale);

        if (color != lastColor)
        {
            flush();
            lastColor = color;

            SDL_SetRenderDrawColor(r, color, color, color, 255);
        }

        batch.push_back(_grid[i]);
    }

    flush();
}