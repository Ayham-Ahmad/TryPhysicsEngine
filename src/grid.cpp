#include "grid.h"

namespace
{
    // The grid cells follow the fluid interaction radius: one cell is exactly
    // one interaction radius wide (the same cells the fluid hash grid uses).
    float gridCellSize()
    {
        return std::max(1.0f, 0.1f * Globals::pixelsPerMeter);
    }
}

// Initialize grid
void Grid::init(const SDL_Rect &box)
{
    float GRID_SIZE = gridCellSize();
    _cellSize = GRID_SIZE;

    int rows = (box.h + GRID_SIZE - 1) / GRID_SIZE;
    int cols = (box.w + GRID_SIZE - 1) / GRID_SIZE;
    _grid.clear();
    _cells.clear();

    for (int i = 0; i < rows; i++)
    {
        float y = box.y + i * GRID_SIZE;
        float height = std::min(GRID_SIZE, box.y + box.h - y);

        for (int j = 0; j < cols; j++)
        {
            float x = box.x + j * GRID_SIZE;
            float width = std::min(GRID_SIZE, box.x + box.w - x);

            SDL_FRect rect = {x, y, width, height};

            _grid.push_back(rect);
            _cells.push_back(0);
        }
    }
}

// Update gird
void Grid::update(const std::vector<ParticleObject> &particles, const SDL_Rect &box)
{
    const float GRID_SIZE = gridCellSize();

    int rows = (box.h + GRID_SIZE - 1) / GRID_SIZE;
    int cols = (box.w + GRID_SIZE - 1) / GRID_SIZE;

    // The cell size follows the interaction radius and the box follows the window,
    // so rebuild the cells whenever either of them changes.
    if (GRID_SIZE != _cellSize || _grid.size() != static_cast<size_t>(rows) * cols)
    {
        init(box);
    }

    std::fill(_cells.begin(), _cells.end(), 0);

    const float ppu = Globals::pixelsPerMeter;
    const float pSize = Globals::particleSize * ppu;

    for (size_t index = 0; index < particles.size(); index++)
    {
        const ParticleObject &p = particles[index];

        // particle positions are in the same space as the box: shift them so the
        // box's top-left corner is cell (0, 0)
        const float px = p.position.x * ppu - box.x;
        const float py = p.position.y * ppu - box.y;

        int minRow = (py - pSize) / GRID_SIZE;
        int maxRow = (py + pSize) / GRID_SIZE;
        int minCol = (px - pSize) / GRID_SIZE;
        int maxCol = (px + pSize) / GRID_SIZE;

        if (minRow < 0) minRow = 0;
        if (minCol < 0) minCol = 0;
        if (maxRow > rows - 1) maxRow = rows - 1;
        if (maxCol > cols - 1) maxCol = cols - 1;

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

    // TODO: instead of mulitple cells mearge them
    auto flush = [&]()
    {
        if (batch.empty())
            return;

        const int count = static_cast<int>(batch.size());

        if (isStarted)
            SDL_RenderFillRects(r, batch.data(), count);
        else
            SDL_RenderRects(r, batch.data(), count);

        batch.clear();
    };

    if (!isStarted)
    {
        SDL_SetRenderDrawColor(r, 100, 255, 100, 255);
        SDL_RenderRects(r, _grid.data(), static_cast<int>(_grid.size()));
        return;
    }

    // Base layer: all cells in one call, so the empty ones stay visible
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderFillRects(r, _grid.data(), static_cast<int>(_grid.size()));

    SDL_SetRenderDrawColor(r, 0, 0, 255, 255);

    for (size_t i = 0; i < _grid.size(); i++)
    {
        // const float scale = std::min(1.0f, static_cast<float>(_cells[i]) / Globals::particlesCount * 0.02f);
        // const int color = static_cast<int>(255.0f * scale);

        // if (color != lastColor)
        // {
        //     flush();
        //     lastColor = color;

        //     SDL_SetRenderDrawColor(r, color, color, color, 255);
        // }

        if (_cells[i] == 0)
            continue;

        batch.push_back(_grid[i]);
    }

    flush();
}
