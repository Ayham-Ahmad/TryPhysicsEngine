#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>

#include "colors.h"
#include "text.h"
#include "globals.h"

struct Slider
{
    std::string id;
    float initialValue = 0.5f;
    float min = 0.0f;
    float max = 1.0f;
    bool dragging = false;

    // Normalized knob position: 0.0 to 1.0.
    float value = 0.5f;
    float y = 0.0f;
};

class Panel
{
public:
    ~Panel();

    void init(float sw);
    void handleEvent(const SDL_Event &event);
    void render();

    float value(const std::string &id) const;
    bool paused() const;

private:
    // Panel layout settings
    static constexpr float PANEL_W = 400.0f;
    static constexpr float PANEL_MARGIN = 20.0f;
    static constexpr float LABEL_X = 10.0f;
    static constexpr float SLIDER_X = 160.0f;
    static constexpr float SLIDER_H = 20.0f;
    static constexpr float SLIDER_TOP = 20.0f;
    static constexpr float SLIDER_SPACING = 50.0f;
    static constexpr float PANEL_BOTTOM = 20.0f;
    static constexpr float BUTTON_X = 20.0f;
    static constexpr float BUTTON_H = 26.0f;
    static constexpr float BUTTON_TOP_MARGIN = 12.0f;

    // Panel helper functions
    void close();
    SDL_FRect knobRect(const Slider &slider) const;
    SDL_WindowID eventWindowID(const SDL_Event &event) const;
    float panelHeight() const;
    float sliderWidth() const;
    static float normalizeValue(float value, float min, float max);
    static float keepbetween0and1(float value);
    void handleSliderValue(Slider &slider, const SDL_Event &event) const;
    void handleKnobEvent(Slider &slider, const SDL_Event &event) const;
    void handlePauseEvent(const SDL_Event &event);

    // Store panel window and renderer
    SDL_Window *window = nullptr;
    SDL_Renderer *r = nullptr;
    SDL_WindowID windowID = 0;

    // Store all sliders
    std::vector<Slider> sliders;

    bool isPaused = true;
    Colors c;
};