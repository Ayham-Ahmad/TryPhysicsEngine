#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cmath>

#include "colors.h"
#include "text.h"
#include "window.h"

struct Slider
{
    std::string id;
    float initialValue = 0.5f;
    float min = 0.0f;
    float max = 1.0f;
    bool listener = false;
    bool dragging = false;
    bool changed = false;

    // Normalized knob position: 0.0 to 1.0.
    float value = 0.5f;
    float y = 0.0f;
};

struct Button
{
    std::string id;
    std::string text;

    bool value = false;
    bool visibility = true;
    bool listener = false;
    bool changed = false;

    float y = 0.0f;
    float x = 0.0f;
};

struct PanelText
{
    std::string id;
    std::string label;
    float value = 0.0f;
    float y = 0.0f;
};

class Panel : public Window
{
public:
    ~Panel();

    void init(float sw);
    void init();
    void handleEvent(const SDL_Event &event);
    void render(const float mouseX, const float mouseY) const;

    float value(const std::string &id) const;
    bool buttonValue(const std::string &id) const;
    bool btnListener(const std::string &id);
    bool sliderListener(const std::string &id);

    void setText(const std::string &id, float value);

    void close();

private:
    // Panel layout settings
    static constexpr float PANEL_W = 400.0f;
    static constexpr float PANEL_MARGIN = 20.0f;
    static constexpr float SECTION_SPACING = 30.0f;
    static constexpr float PANEL_BOTTOM = 20.0f;

    // Section 1: text lines
    static constexpr float TEXT_X = 10.0f;
    static constexpr float TEXT_TOP = 20.0f;
    static constexpr float TEXT_SPACING = 24.0f;

    // Section 2: action buttons (one line of 3)
    static constexpr float BUTTON_H = 26.0f;
    static constexpr float BUTTON_GAP = 8.0f;

    // Section 3: checkboxes (two columns)
    static constexpr float CHECKBOX_SIZE = 16.0f;
    static constexpr float BUTTON_X = 20.0f;
    static constexpr float CHECKBOX_ROW_SPACING = 40.0f;

    // Section 4: sliders
    static constexpr float LABEL_X = 10.0f;
    static constexpr float SLIDER_X = 160.0f;
    static constexpr float SLIDER_H = 20.0f;
    static constexpr float SLIDER_ROW_SPACING = 40.0f;

    // Panel helper functions
    static SDL_FRect knobRect(const Slider &slider);
    float panelHeight() const;
    static float sliderWidth();
    static float actionButtonWidth();
    int actionButtonCount() const;
    static bool isActionButton(const std::string &id);
    static float normalizeValue(float value, float min, float max);
    static float clamp01(float value);
    static void handleSliderValue(Slider &slider, const SDL_Event &event);
    static void handleKnobEvent(Slider &slider, const SDL_Event &event);
    void handleButtonEvent(const SDL_Event &event);
    void setValue(const std::string &id, bool value);
    void setVisibility(const std::string &id, bool visibility);
    static bool isMouseInRect(const SDL_FRect &rect, const float mouseX, const float mouseY);

    // Store all texts, sliders and buttons
    std::vector<PanelText> _texts;
    std::vector<Slider> _sliders;
    std::vector<Button> _buttons;
};