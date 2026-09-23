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

    // The debug grid is in pixels, the sim is in meters
    const float ppu = Globals::pixelsPerMeter;

    int rows = (Globals::sh + Globals::gridSize - 1) / Globals::gridSize;
    int cols = (Globals::sw + Globals::gridSize - 1) / Globals::gridSize;

    // Number of per-thread accumulators
    unsigned threads = std::thread::hardware_concurrency();
    if (threads == 0)
        threads = 1;

    // Every thread writes its OWN copy of the cell counts
    std::vector<std::vector<int>> partial(threads, std::vector<int>(_cells.size(), 0));

    parallelChunks(particles.size(), [&](const size_t begin, const size_t end, const unsigned threadId)
    {
        std::vector<int> &cells = partial[threadId];

        for (size_t index = begin; index < end; index++)
        {
            const ParticleObject &p = particles[index];

            const float px = p.position.x * ppu;
            const float py = p.position.y * ppu;
            const float pSize = Globals::particleSize * ppu;

            int minRow = (py - pSize) / Globals::gridSize;
            int maxRow = (py + pSize) / Globals::gridSize;
            int minCol = (px - pSize) / Globals::gridSize;
            int maxCol = (px + pSize) / Globals::gridSize;

            if (minRow < 0) minRow = 0;
            if (maxRow >= rows) maxRow = rows - 1;
            if (minCol < 0) minCol = 0;
            if (maxCol >= cols) maxCol = cols - 1;

            for (int row = minRow; row <= maxRow; row++)
            {
                for (int col = minCol; col <= maxCol; col++)
                {
                    cells[row * cols + col]++;
                }
            }
        }
    });

    // Merge all threads' buckets into _cells (single threaded)
    for (size_t i = 0; i < _cells.size(); i++)
    {
        for (unsigned threadId = 0; threadId < threads; threadId++)
            _cells[i] += partial[threadId][i];
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