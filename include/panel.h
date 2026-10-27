#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <cstdio>
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
    bool blockDuringSim = false;
    bool dragging = false;
    bool changed = false;
    float value = 0.5f;
    float y = 0.0f;
    std::string textBuffer;
    std::string cachedText;
    bool editing = false;
    bool selectAll = false;
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

    void init(float sw, SDL_Window* parent = nullptr);
    void init();
    void handleEvent(const SDL_Event &event);
    void render(const float mouseX, const float mouseY);

    float value(const std::string &id) const;
    bool buttonValue(const std::string &id) const;
    bool btnListener(const std::string &id);
    bool sliderListener(const std::string &id);
    bool isEditing() const;

    void setText(const std::string &id, float value);

    void close();

    // Keep these public so code can push_back/edit them directly
    std::vector<PanelText> _texts;
    std::vector<Slider> _sliders;
    std::vector<Button> _buttons;

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

    // Section 2: action buttons
    static constexpr float BUTTON_H = 26.0f;
    static constexpr float BUTTON_GAP = 8.0f;

    // Section 3: checkboxes
    static constexpr float CHECKBOX_SIZE = 16.0f;
    static constexpr float BUTTON_X = 20.0f;
    static constexpr float CHECKBOX_ROW_SPACING = 40.0f;

    // Section 4: sliders
    static constexpr float LABEL_X = 10.0f;
    static constexpr float SLIDER_X = 160.0f;
    static constexpr float SLIDER_H = 20.0f;
    static constexpr float SLIDER_ROW_SPACING = 40.0f;

    // Inline value box
    static constexpr float VALUE_BOX_W = 80.0f;
    static constexpr float VALUE_BOX_GAP = 6.0f;
    static constexpr float VALUE_BOX_PADDING = 5.0f;
    static constexpr int VALUE_FONT_SIZE = 12;

    void rebuildMaps();
    void rebuildLayout();

    static SDL_FRect knobRectFrom(const Slider &slider);
    float panelHeight() const;
    static float sliderWidth();
    static float trackWidth();
    bool sliderLocked(int idx) const;
    static SDL_FRect valueBoxRectFrom(const Slider &slider);
    static bool parseNumber(const std::string &text, float &result);
    void beginEditing(int index);
    void endEditing(bool commit);
    void commitEditing();
    bool handleTextBoxClick(const SDL_Event &event, float mx, float my);
    bool handleTextBoxInput(const SDL_Event &event);
    static float actionButtonWidth();
    int actionButtonCount() const;
    static bool isActionButton(const std::string &id);
    static float normalizeValue(float value, float min, float max);
    static float clamp01(float value);
    void handleSliderValueDrag(int idx, const SDL_Event &event);
    void handleKnobEventClick(int idx, const SDL_Event &event);
    void handleButtonEvent(const SDL_Event &event);
    void setValue(const std::string &id, bool value);
    void setVisibility(const std::string &id, bool visibility);
    static bool isMouseInRect(const SDL_FRect &rect, const float mouseX, const float mouseY);
    static std::string fmtFloat(float v);

    std::unordered_map<std::string, int> _textIdx;
    std::unordered_map<std::string, int> _sliderIdx;
    std::unordered_map<std::string, int> _btnIdx;

    std::vector<SDL_FRect> _trackR;
    std::vector<SDL_FRect> _knobR;
    std::vector<SDL_FRect> _boxR;
    std::vector<SDL_FRect> _btnR;
    std::vector<bool> _sliderLockedCache;

    float _slidersY = 0.0f;
    int _checkboxRows = 0;
    int _activeSlider = -1;
    int _editingIndex = -1;
    float _sw = 0.0f;
    SDL_Window* _parent = nullptr;
};