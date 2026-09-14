#pragma once

#include <SDL3/SDL.h>

#include "text.h"

// Base class for every class that owns its own window and renderer
class Window
{
public:
    bool isOpen() const
    {
        return window != nullptr;
    }

    // Destroy the window and its renderer (clears cached text first)
    void close()
    {
        purgeRendererTextures(r);

        if (r)
        {
            SDL_DestroyRenderer(r);
            r = nullptr;
        }

        if (window)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        windowID = 0;
    }

    // Get the window ID of an event
    SDL_WindowID eventWindowID(const SDL_Event &event) const
    {
        switch (event.type)
        {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            return event.window.windowID;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return event.button.windowID;

        case SDL_EVENT_MOUSE_MOTION:
            return event.motion.windowID;

        default:
            return 0;
        }
    }

protected:
    SDL_Window *window = nullptr;
    SDL_Renderer *r = nullptr;
    SDL_WindowID windowID = 0;
};