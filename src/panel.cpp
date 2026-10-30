#include "panel.h"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <string>

static std::string fmtf(float v)
{
    char buf[32];
    std::snprintf(buf, 32, "%.3f", v);
    std::string s(buf);
    while (s.size() > 1 && s.back() == '0' && s.find('.') != std::string::npos)
        s.pop_back();
    if (s.back() == '.')
        s.pop_back();
    return s;
}

Panel::~Panel() { close(); }

void Panel::init(float sw, SDL_Window* parent) { _sw = sw; _parent = parent; init(); }
void Panel::init()
{
    _texts = {
        {"count", "Count: "},
        {"totalCount", "Total count: "},
        {"res", "Res: "},
    };

    for (size_t ti = 0; ti < _texts.size(); ti++) _texts[ti].y = TEXT_TOP + (float)ti * TEXT_SPACING;

    _buttons = {
        {"start", "Start", false, true},
        {"reset", "Reset", false, false, true},
        {"pause", "Pause", false, false},
        {"grid", "Grid", false, true},
        {"graph", "Graph", false, true},
        {"gravity", "Gravity", true, true},
        {"screenCollision", "Screen Collision", true, true},
        {"particles", "Show Particles", true, true},
        {"circles", "Render Circles", false, true},
        // {"gradientArrows", "Gradient Arrows", false, true},
        {"speedColoring", "Speed Colors", true, true},
        {"emitters", "Emitters", false, true},
        {"random", "random", false, true, true},
    };

    _sliders = {
        {"speed", 0.5f, 0.1f, 5.0f, false, false},
        {"gravity", 9.8f, 0.0f, 20.0f, true, false},
        {"count", 500.0f, 1.0f, 40000.0f, true, true},
        {"spacing", 0.001f, 0.001f, 0.1f, true, true},
        {"size", 0.031f, 0.002f, 0.05f, true, true},
        // {"smoothingRadius", 0.5f, 0.01f, 2.0f, true, false},
        // {"mass", 0.4f, 0.01f, 10.0f, true, false},
        {"target_density", 0.624, 0.1f, 20.0f, true, false},
        {"kNear", 30.0f, 0.1f, 100.0f, true, false},
        {"k", 49.0f, 0.1f, 200.0f, true, false},
        {"sigma", 0.01f, 0.0f, 10.0f, true, false},
        {"beta", 0.01f, 0.0f, 10.0f, true, false},
    };

    for (auto &s : _sliders)
    {
        s.value = normalizeValue(s.initialValue, s.min, s.max);
        s.dragging = false;
        s.changed = false;
        s.editing = false;
        s.selectAll = false;
        s.textBuffer.clear();
        s.cachedText = fmtf(s.initialValue);
    }
    for (auto &simulationValues : _simSliderValues)
    {
        simulationValues.resize(_sliders.size());
        for (size_t i = 0; i < _sliders.size(); i++)
            simulationValues[i] = _sliders[i].value;
    }
    for (auto &simulationChanges : _simSliderChanged)
        simulationChanges.assign(_sliders.size(), false);
    for (auto &b : _buttons) { b.changed = false; }

    rebuildMaps();
    for (size_t ti = 0; ti < _texts.size(); ti++) _texts[ti].y = TEXT_TOP + (float)ti * TEXT_SPACING;
    rebuildLayout();

    window = SDL_CreateWindow("Controls", (int)PANEL_W, (int)panelHeight(), SDL_WINDOW_RESIZABLE);
    r = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    windowID = SDL_GetWindowID(window);

    if (_sw <= 0.0f)
    {
        SDL_DisplayID d = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode *m = SDL_GetCurrentDisplayMode(d);
        if (m) _sw = (float)m->w;
    }

    SDL_SetWindowPosition(window, (int)(_sw - PANEL_W), 40);
    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);
    SDL_SetWindowAlwaysOnTop(window, true);
}

void Panel::rebuildMaps()
{
    _textIdx.clear(); for (size_t i = 0; i < _texts.size(); i++) _textIdx[_texts[i].id] = (int)i;
    _sliderIdx.clear(); for (size_t i = 0; i < _sliders.size(); i++) _sliderIdx[_sliders[i].id] = (int)i;
    _btnIdx.clear(); for (size_t i = 0; i < _buttons.size(); i++) _btnIdx[_buttons[i].id] = (int)i;
}

void Panel::rebuildLayout()
{
    const float textsBottom = TEXT_TOP + (float)_texts.size() * TEXT_SPACING;
    _simTabsY = textsBottom + SECTION_SPACING;
    const float tabWidth = (PANEL_W - PANEL_MARGIN * 2.0f) / SIM_COUNT;
    for (int i = 0; i < SIM_COUNT; i++)
        _simTabR[i] = {PANEL_MARGIN + i * tabWidth, _simTabsY, tabWidth - BUTTON_GAP, SIM_TAB_H};
    const float buttonsY = _simTabsY + SIM_TAB_H + SECTION_SPACING;
    int ai = 0;
    for (size_t i = 0; i < _buttons.size(); i++)
    {
        auto &b = _buttons[i];
        if (!isActionButton(b.id)) continue;
        b.x = PANEL_MARGIN + (float)ai * (actionButtonWidth() + BUTTON_GAP);
        b.y = buttonsY;
        ai++;
    }
    const float checkboxesY = buttonsY + BUTTON_H + SECTION_SPACING;
    int ci = 0;
    for (size_t i = 0; i < _buttons.size(); i++)
    {
        auto &b = _buttons[i];
        if (isActionButton(b.id)) continue;
        int row = ci / 2;
        int col = ci % 2;
        b.x = col == 0 ? BUTTON_X : PANEL_W / 2.0f;
        b.y = checkboxesY + (float)row * CHECKBOX_ROW_SPACING;
        ci++;
    }
    _checkboxRows = (ci + 1) / 2;
    _slidersY = checkboxesY + (float)_checkboxRows * CHECKBOX_ROW_SPACING + SECTION_SPACING;
    for (size_t i = 0; i < _sliders.size(); i++) _sliders[i].y = _slidersY + (float)i * SLIDER_ROW_SPACING;

    _trackR.resize(_sliders.size());
    _knobR.resize(_sliders.size());
    _boxR.resize(_sliders.size());
    _btnR.resize(_buttons.size());
    _sliderLockedCache.resize(_sliders.size());
    const bool started = buttonValue("start");
    for (size_t i = 0; i < _sliders.size(); i++)
    {
        const auto &s = _sliders[i];
        _trackR[i] = {SLIDER_X, s.y + SLIDER_H / 3.0f, trackWidth(), SLIDER_H / 3.0f};
        _knobR[i] = {SLIDER_X + s.value * (trackWidth() - SLIDER_H), s.y, SLIDER_H, SLIDER_H};
        _boxR[i] = {SLIDER_X + trackWidth() + VALUE_BOX_GAP, s.y, VALUE_BOX_W, SLIDER_H};
        _sliderLockedCache[i] = _sliders[i].blockDuringSim && started;
    }
    for (size_t i = 0; i < _buttons.size(); i++)
    {
        const auto &b = _buttons[i];
        if (isActionButton(b.id)) _btnR[i] = {b.x, b.y, actionButtonWidth(), BUTTON_H};
        else _btnR[i] = {b.x, b.y, CHECKBOX_SIZE, CHECKBOX_SIZE};
    }
}

float Panel::panelHeight() const
{
    if (_texts.empty() && _sliders.empty() && _buttons.empty()) return PANEL_MARGIN * 2.0f;
    const float textsBottom = TEXT_TOP + (float)_texts.size() * TEXT_SPACING;
    const float buttonsY = textsBottom + SECTION_SPACING + SIM_TAB_H + SECTION_SPACING;
    const float checkboxesY = buttonsY + BUTTON_H + SECTION_SPACING;
    int cb = 0;
    for (const auto &b : _buttons) if (!isActionButton(b.id)) cb++;
    int cbRows = (cb + 1) / 2;
    const float slidersY = checkboxesY + (float)cbRows * CHECKBOX_ROW_SPACING + SECTION_SPACING;
    if (_sliders.empty()) return slidersY + SLIDER_H + PANEL_BOTTOM;
    return slidersY + (float)(_sliders.size()-1) * SLIDER_ROW_SPACING + SLIDER_H + PANEL_BOTTOM;
}

bool Panel::isMouseInRect(const SDL_FRect &rect, const float mx, const float my)
{
    return mx >= rect.x && mx <= rect.x + rect.w && my >= rect.y && my <= rect.y + rect.h;
}

float Panel::actionButtonWidth() { return (PANEL_W - PANEL_MARGIN * 2.0f - BUTTON_GAP * 2.0f) / 3.0f; }
int Panel::actionButtonCount() const { int c = 0; for (const auto &b : _buttons) if (isActionButton(b.id)) c++; return c; }
bool Panel::isActionButton(const std::string &id) { return id == "start" || id == "reset" || id == "pause"; }
float Panel::sliderWidth() { return PANEL_W - SLIDER_X - PANEL_MARGIN; }
float Panel::trackWidth() { return sliderWidth() - VALUE_BOX_W - VALUE_BOX_GAP; }
float Panel::normalizeValue(float v, float min, float max) { if (max == min) return 0.0f; return (v - min) / (max - min); }
float Panel::clamp01(float v) { if (v < 0.0f) return 0.0f; if (v > 1.0f) return 1.0f; return v; }
bool Panel::isSimulationSpecificSlider(const std::string &id)
{
    return id != "speed" && id != "count" && id != "size" &&
           id != "spacing" && id != "target_density";
}
bool Panel::sliderLocked(int idx) const { if (idx < 0 || idx >= (int)_sliderLockedCache.size()) return false; return _sliderLockedCache[idx]; }
SDL_FRect Panel::knobRectFrom(const Slider &s) { return {SLIDER_X + s.value * (trackWidth() - SLIDER_H), s.y, SLIDER_H, SLIDER_H}; }
SDL_FRect Panel::valueBoxRectFrom(const Slider &s) { return {SLIDER_X + trackWidth() + VALUE_BOX_GAP, s.y, VALUE_BOX_W, SLIDER_H}; }
bool Panel::parseNumber(const std::string &text, float &result)
{
    if (text.empty()) return false;
    char *end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (end != text.c_str() + text.size()) return false;
    result = (float)parsed;
    return std::isfinite(result);
}
bool Panel::isEditing() const { return _editingIndex >= 0; }

float Panel::value(const std::string &id) const
{
    auto it = _sliderIdx.find(id);
    if (it == _sliderIdx.end()) return 0.0f;
    const auto &s = _sliders[it->second];
    return s.min + s.value * (s.max - s.min);
}

float Panel::value(const std::string &id, int simulationIndex) const
{
    auto it = _sliderIdx.find(id);
    if (it == _sliderIdx.end() || simulationIndex < 0 || simulationIndex >= SIM_COUNT)
        return 0.0f;

    const int sliderIndex = it->second;
    const auto &slider = _sliders[sliderIndex];
    const float normalizedValue = isSimulationSpecificSlider(slider.id)
                                      ? _simSliderValues[simulationIndex][sliderIndex]
                                      : slider.value;
    return slider.min + normalizedValue * (slider.max - slider.min);
}

int Panel::activeSimulation() const
{
    return _activeSimulation;
}

void Panel::setActiveSimulation(int simulationIndex)
{
    if (simulationIndex < 0 || simulationIndex >= SIM_COUNT || simulationIndex == _activeSimulation)
        return;

    endEditing(true);
    if (_activeSlider >= 0 && _activeSlider < (int)_sliders.size())
        _sliders[_activeSlider].dragging = false;
    _activeSlider = -1;
    _activeSimulation = simulationIndex;

    for (size_t i = 0; i < _sliders.size(); i++)
    {
        Slider &slider = _sliders[i];
        const bool simulationSpecific = isSimulationSpecificSlider(slider.id);
        if (simulationSpecific)
            slider.value = _simSliderValues[_activeSimulation][i];
        if (simulationSpecific)
            slider.changed = false;
        slider.dragging = false;
        slider.cachedText = fmtf(value(slider.id));
    }

    _editingIndex = -1;
    rebuildLayout();
}

bool Panel::buttonValue(const std::string &id) const
{
    auto it = _btnIdx.find(id);
    if (it == _btnIdx.end()) return false;
    return _buttons[it->second].value;
}

void Panel::setText(const std::string &id, float v)
{
    auto it = _textIdx.find(id);
    if (it == _textIdx.end()) return;
    _texts[it->second].value = v;
}

bool Panel::btnListener(const std::string &id)
{
    auto it = _btnIdx.find(id);
    if (it == _btnIdx.end()) return false;
    if (_buttons[it->second].listener && _buttons[it->second].changed) { _buttons[it->second].changed = false; return true; }
    return false;
}

bool Panel::sliderListener(const std::string &id)
{
    return sliderListener(id, _activeSimulation);
}

bool Panel::sliderListener(const std::string &id, int simulationIndex)
{
    auto it = _sliderIdx.find(id);
    if (it == _sliderIdx.end() || simulationIndex < 0 || simulationIndex >= SIM_COUNT)
        return false;
    int idx = it->second;
    if (!isSimulationSpecificSlider(id))
    {
        if (_sliders[idx].listener && _sliders[idx].changed)
        {
            _sliders[idx].changed = false;
            return true;
        }
        return false;
    }

    if (_sliders[idx].listener && _simSliderChanged[simulationIndex][idx])
    {
        _simSliderChanged[simulationIndex][idx] = false;
        if (simulationIndex == _activeSimulation)
            _sliders[idx].changed = false;
        return true;
    }
    return false;
}

void Panel::setValue(const std::string &id, bool v)
{
    auto it = _btnIdx.find(id);
    if (it == _btnIdx.end()) return;
    _buttons[it->second].value = v;
}

void Panel::setVisibility(const std::string &id, bool v)
{
    auto it = _btnIdx.find(id);
    if (it == _btnIdx.end()) return;
    _buttons[it->second].visibility = v;
}

void Panel::beginEditing(int index)
{
    if (index < 0 || index >= (int)_sliders.size()) return;
    if (_editingIndex == index) return;
    if (sliderLocked(index)) return;
    Slider &s = _sliders[index];
    s.textBuffer = fmtf(value(_sliders[index].id));
    s.selectAll = true;
    s.editing = true;
    _editingIndex = index;
    SDL_StartTextInput(window);
}

void Panel::endEditing(bool commit)
{
    if (_editingIndex < 0) return;
    if (commit) commitEditing();
    _sliders[_editingIndex].editing = false;
    _sliders[_editingIndex].selectAll = false;
    _sliders[_editingIndex].textBuffer.clear();
    _editingIndex = -1;
    SDL_StopTextInput(window);
}

void Panel::commitEditing()
{
    if (_editingIndex < 0) return;
    Slider &s = _sliders[_editingIndex];
    float typed = 0.0f;
    if (!parseNumber(s.textBuffer, typed)) return;
    typed = std::clamp(typed, s.min, s.max);
    float prev = value(s.id);
    s.value = normalizeValue(typed, s.min, s.max);
    s.cachedText = fmtf(typed);
    if (s.listener && value(s.id) != prev) s.changed = true;
    if (isSimulationSpecificSlider(s.id))
    {
        _simSliderValues[_activeSimulation][_editingIndex] = s.value;
        if (s.listener && value(s.id) != prev)
            _simSliderChanged[_activeSimulation][_editingIndex] = true;
    }
    _knobR[_editingIndex] = {SLIDER_X + s.value * (trackWidth() - SLIDER_H), s.y, SLIDER_H, SLIDER_H};
}

void Panel::handleSliderValueDrag(int idx, const SDL_Event &e)
{
    if (idx < 0 || idx >= (int)_sliders.size()) return;
    if (sliderLocked(idx)) return;
    Slider &s = _sliders[idx];
    float old = s.value;
    s.value = (e.motion.x - SLIDER_X) / (trackWidth() - SLIDER_H);
    s.value = clamp01(s.value);
    _knobR[idx] = {SLIDER_X + s.value * (trackWidth() - SLIDER_H), s.y, SLIDER_H, SLIDER_H};
    if (s.listener && s.value != old) s.changed = true;
    if (isSimulationSpecificSlider(s.id))
    {
        _simSliderValues[_activeSimulation][idx] = s.value;
        if (s.listener && s.value != old)
            _simSliderChanged[_activeSimulation][idx] = true;
    }
}

void Panel::handleKnobEventClick(int idx, const SDL_Event &e)
{
    if (idx < 0 || idx >= (int)_sliders.size()) return;
    if (sliderLocked(idx)) return;
    _sliders[idx].dragging = isMouseInRect(_knobR[idx], e.button.x, e.button.y);
    _activeSlider = _sliders[idx].dragging ? idx : -1;
}

void Panel::handleButtonEvent(const SDL_Event &e)
{
    int cb = 0;
    for (size_t i = 0; i < _buttons.size(); i++)
    {
        if (!_buttons[i].visibility) continue;
        if (!isMouseInRect(_btnR[i], e.button.x, e.button.y)) continue;
        auto &b = _buttons[i];
        if (b.id == "start")
        {
            b.value = true; b.changed = b.listener;
            setVisibility("start", false); setVisibility("reset", true); setVisibility("pause", true);
            const bool started = true;
            for (size_t j = 0; j < _sliders.size(); j++) _sliderLockedCache[j] = _sliders[j].blockDuringSim && started;
            return;
        }
        if (b.id == "reset")
        {
            b.value = true;
            b.changed = b.listener;
            setVisibility("start", true);
            setVisibility("reset", false);
            setVisibility("pause", false);
            for (size_t bi = 0; bi < _buttons.size(); bi++)
                if (_buttons[bi].id == "pause") { _buttons[bi].value = false; _buttons[bi].changed = false; }
            if (auto *st = _btnIdx.count("start") ? &_buttons[_btnIdx["start"]] : nullptr) { st->value = false; }
            const bool started = false;
            for (size_t j = 0; j < _sliders.size(); j++) _sliderLockedCache[j] = _sliders[j].blockDuringSim && started;
            b.value = false;
            return;
        }
        if (b.id == "pause")
        {
            b.value = !b.value; b.changed = b.listener;
            return;
        }
        if (b.id == "random")
        {
            b.value = !b.value;
            b.changed = b.listener;
            return;
        }
        b.value = !b.value; b.changed = b.listener;
        cb++;
    }
}

bool Panel::handleTextBoxClick(const SDL_Event &e, float mx, float my)
{
    for (int i = 0; i < (int)_sliders.size(); i++)
    {
        if (sliderLocked(i)) continue;
        if (!isMouseInRect(_boxR[i], mx, my)) continue;
        if (_editingIndex >= 0 && _editingIndex != i) endEditing(true);
        beginEditing(i);
        return true;
    }
    return false;
}

bool Panel::handleTextBoxInput(const SDL_Event &e)
{
    if (_editingIndex < 0) return false;
    Slider &s = _sliders[_editingIndex];
    if (e.type == SDL_EVENT_TEXT_INPUT)
    {
        if (s.selectAll) { s.textBuffer.clear(); s.selectAll = false; }
        for (const char *c = e.text.text; *c; c++)
        {
            bool ok = (*c >= '0' && *c <= '9') || *c == '.' || *c == '-';
            if (ok && s.textBuffer.size() < 12) s.textBuffer.push_back(*c);
        }
        return true;
    }
    if (e.type != SDL_EVENT_KEY_DOWN) return false;
    if (e.key.scancode == SDL_SCANCODE_BACKSPACE)
    {
        if (s.selectAll) { s.textBuffer.clear(); s.selectAll = false; }
        else if (!s.textBuffer.empty()) s.textBuffer.pop_back();
        return true;
    }
    if (e.key.scancode == SDL_SCANCODE_RETURN || e.key.scancode == SDL_SCANCODE_KP_ENTER) { endEditing(true); return true; }
    if (e.key.scancode == SDL_SCANCODE_ESCAPE) { endEditing(false); return true; }
    if (e.key.scancode == SDL_SCANCODE_TAB)
    {
        int cur = _editingIndex;
        endEditing(true);
        int step = (e.key.mod & SDL_KMOD_SHIFT) ? -1 : 1;
        for (int ni = cur + step; ni >= 0 && ni < (int)_sliders.size(); ni += step)
        {
            if (sliderLocked(ni)) continue;
            beginEditing(ni);
            break;
        }
        return true;
    }
    return true;
}

bool Panel::handleSimulationTabClick(float mouseX, float mouseY)
{
    for (int i = 0; i < SIM_COUNT; i++)
    {
        if (isMouseInRect(_simTabR[i], mouseX, mouseY))
        {
            setActiveSimulation(i);
            return true;
        }
    }
    return false;
}

void Panel::handleEvent(const SDL_Event &e)
{
    if (!window || eventWindowID(e) != windowID) return;
    if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) { close(); return; }
    if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST) { endEditing(false); return; }
    if (e.type == SDL_EVENT_WINDOW_FOCUS_GAINED) { if (_editingIndex >= 0) SDL_StartTextInput(window); return; }

    if (_editingIndex >= 0 && handleTextBoxInput(e)) return;

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        if (handleTextBoxClick(e, e.button.x, e.button.y)) return;
        if (handleSimulationTabClick(e.button.x, e.button.y))
        {
            endEditing(true);
            return;
        }
        endEditing(true);
        handleButtonEvent(e);
        if (_slidersY <= 0.0f) return;
        int idx = (int)((e.button.y - _slidersY) / SLIDER_ROW_SPACING);
        if (idx >= 0 && idx < (int)_sliders.size() && !sliderLocked(idx))
        {
            if (isMouseInRect(_knobR[idx], e.button.x, e.button.y) || e.button.x >= SLIDER_X && e.button.x <= SLIDER_X + trackWidth() + VALUE_BOX_W + VALUE_BOX_GAP)
            {
                if (!isMouseInRect(_boxR[idx], e.button.x, e.button.y))
                    handleKnobEventClick(idx, e);
            }
        }
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT)
    {
        if (_activeSlider >= 0 && _activeSlider < (int)_sliders.size()) _sliders[_activeSlider].dragging = false;
        _activeSlider = -1;
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        if (_activeSlider >= 0 && _activeSlider < (int)_sliders.size() && _sliders[_activeSlider].dragging)
        {
            handleSliderValueDrag(_activeSlider, e);
            return;
        }
    }
}

void Panel::render(const float mouseX, const float mouseY)
{
    if (!window || !r) return;
    SDL_SetRenderTarget(r, nullptr);
    Colors::VeryDarkGray(r);
    SDL_RenderClear(r);

    for (const auto &t : _texts)
    {
        std::string s = t.label;
        s += fmtf(t.value);
        drawText(r, s, TEXT_X, t.y, 14);
    }

    for (int i = 0; i < SIM_COUNT; i++)
    {
        const auto &tab = _simTabR[i];
        if (i == _activeSimulation)
            Colors::MustardYellow(r);
        else if (isMouseInRect(tab, mouseX, mouseY))
            Colors::MediumGray(r);
        else
            Colors::DarkGray(r);
        SDL_RenderFillRect(r, &tab);
        SDL_SetRenderDrawColorFloat(r, 0.1f, 0.1f, 0.1f, 1.0f);
        SDL_RenderRect(r, &tab);
        drawText(r, std::to_string(i + 1), tab.x + 10.0f, tab.y + 5.0f, 14);
    }

    for (size_t i = 0; i < _buttons.size(); i++)
    {
        if (!_buttons[i].visibility) continue;
        const auto &b = _buttons[i];
        const auto &br = _btnR[i];
        if (isActionButton(b.id))
        {
            if (b.id == "start") Colors::Green(r);
            else if (b.id == "reset") Colors::Red(r);
            else if (b.id == "pause") Colors::Orange(r);
            else Colors::DarkGray(r);
            SDL_RenderFillRect(r, &br);
            drawText(r, b.text, br.x + 10.0f, br.y + 4.0f, 16);
        }
        else
        {
            Colors::VeryDarkGray(r);
            SDL_RenderFillRect(r, &br);
            SDL_RenderRect(r, &br);
            if (b.value)
            {
                SDL_FRect chk = {br.x + 3.0f, br.y + 3.0f, br.w - 6.0f, br.h - 6.0f};
                Colors::Green(r);
                SDL_RenderFillRect(r, &chk);
            } else {
                SDL_FRect chk = {br.x + 3.0f, br.y + 3.0f, br.w - 6.0f, br.h - 6.0f};
                SDL_SetRenderDrawColorFloat(r, 0.2f, 0.2f, 0.2f, 1.0f);
                SDL_RenderFillRect(r, &chk);
            }
            drawText(r, b.text, br.x + CHECKBOX_SIZE + 8.0f, br.y + 1.0f, 14);
        }
    }

    const bool started = buttonValue("start");
    for (size_t i = 0; i < _sliders.size(); i++)
    {
        const auto &s = _sliders[i];
        const bool locked = _sliderLockedCache[i];
        SDL_SetRenderDrawColorFloat(r, 0.3f, 0.3f, 0.3f, 1.0f);
        SDL_RenderFillRect(r, &_trackR[i]);
        if (locked) Colors::DarkGray(r);
        else Colors::MustardYellow(r);
        SDL_RenderFillRect(r, &_knobR[i]);
        drawText(r, s.id, LABEL_X, s.y, 14);
        const auto &box = _boxR[i];
        SDL_SetRenderDrawColorFloat(r, 0.15f, 0.15f, 0.15f, 1.0f);
        SDL_RenderFillRect(r, &box);
        const bool hovered = isMouseInRect(box, mouseX, mouseY);
        if (locked) Colors::DarkGray(r);
        else if (s.editing) Colors::MustardYellow(r);
        else if (hovered) Colors::White(r);
        else Colors::MediumGray(r);
        SDL_RenderRect(r, &box);
        const std::string txt = s.editing ? s.textBuffer : fmtf(value(s.id));
        const float tx = box.x + VALUE_BOX_PADDING;
        const float ty = box.y + (box.h - VALUE_FONT_SIZE) / 2.0f - 1.0f;
        drawText(r, txt, tx, ty, VALUE_FONT_SIZE);
        if (s.editing && ((SDL_GetTicks() / 500) % 2 == 0))
        {
            int tw = 0, th = 0;
            if (TTF_GetStringSize(cachedFont(VALUE_FONT_SIZE), txt.c_str(), txt.size(), &tw, &th))
            {
                SDL_FRect caret = {tx + (float)tw + 1.0f, box.y + 4.0f, 1.0f, box.h - 8.0f};
                Colors::White(r);
                SDL_RenderFillRect(r, &caret);
            }
        }
    }
    (void)started;
    SDL_RenderPresent(r);
}

void Panel::close()
{
    endEditing(false);
    Window::close();
    for (auto &s : _sliders) { s.dragging = false; s.changed = false; }
    for (auto &b : _buttons) b.changed = false;
    _activeSlider = -1;
    _editingIndex = -1;
}
