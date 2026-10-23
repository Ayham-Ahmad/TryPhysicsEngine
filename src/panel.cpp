#include "panel.h"

#include <algorithm>
#include <cstdlib>

std::string sliderValueText(const float value)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << value;

    std::string text = ss.str();
    if (const size_t decimalPoint = text.find('.'); decimalPoint != std::string::npos)
    {
        while (text.back() == '0')
            text.pop_back();
        if (text.back() == '.')
            text.pop_back();
    }

    return text;
}

Panel::~Panel()
{
    close();
}

void Panel::init(float sw)
{
    // Initialize the text lines: {id, label}
    _texts = {
        {"entropy", "Entropy: "},
        {"speed", "Speed: "},
        {"kineticEnergy", "Kinetic Energy: "},
        {"acceleration", "Acceleration: "},
        {"density", "Density: "},
    };

    // Initialize the sliders: {id, initValue, min, max, listener, blockDuringSim}
    _sliders = {
        {"speed", 1.0f, 0.1f, 3.0f, false, false},
        {"gravity", 9.8f, 0.0f, 100.0f, false, false},
        {"count", 9000.774f, 1.0f, 40000.0f, true, true},
        {"spacing", 0.02f, 0.005f, 0.05f, true, true},
        {"size", 0.004f, 0.002f, 0.02f, true, true},
        {"smoothingRadius", 0.5f, 0.01f, 1.0f, true, false},
        {"mass", 0.4f, 0.05f, 2.0f, true, false},

        // --- Fluid solver (double density relaxation) ---
        {"interatctionRadius", 0.078f, 0.06f, 0.30f, true, false},
        {"rest_density", 4.05f, 0.0f, 5.0f, true, false},
        {"kNear", 30.0f, 0.0f, 2000.0f, true, false},
        {"k", 65.0f, 0.0f, 1000.0f, true, false},

        // --- Unused by the current solver (kept for the legacy SPH path) ---
        // {"gridSize", 100.0f, 1.0f, 250.0f, true},       // the grid now follows interatctionRadius
        // {"targetDensity", 1000.0f, 1.0f, 2000.0f, true},
        // {"pressure", 0.5f, 0.0f, 10.0f, true},
    };

    // Initialize the buttons: {id, text, value, visibility, listener}
    _buttons = {
        {"start", "Start", false, true},
        {"reset", "Reset", false, false, true},
        {"pause", "Pause", false, false},
        {"grid", "Grid", false, true},
        {"graph", "Graph", false, true},
        {"gravity", "Gravity", true, true},
        {"screenCollision", "Screen Collision", true, true},
        {"particles", "Show Particles", true, true},
        {"circles", "Render Circles", true, true},
        {"gradientArrows", "Gradient Arrows", false, true},
        {"speedColoring", "Speed Colors", true, true},
        {"random", "random", false, true, true},
    };

    // --- Layout top to bottom: texts, action buttons, checkboxes, sliders ---

    // Section 1: text lines
    for (size_t i = 0; i < _texts.size(); i++)
        _texts[i].y = TEXT_TOP + static_cast<float>(i) * TEXT_SPACING;

    // Section 2: start/reset/pause, all on one line
    const float textsBottom = TEXT_TOP + static_cast<float>(_texts.size()) * TEXT_SPACING;
    const float buttonsY = textsBottom + SECTION_SPACING;

    int i = 0;
    for (Button &button : _buttons)
    {
        if (!isActionButton(button.id))
            continue;

        button.x = PANEL_MARGIN + i * (actionButtonWidth() + BUTTON_GAP);
        button.y = buttonsY;
        i++;
    }

    // Section 3: checkboxes, two columns
    const float checkboxesY = buttonsY + BUTTON_H + SECTION_SPACING;

    int checkboxIndex = 0;
    for (Button &button : _buttons)
    {
        if (isActionButton(button.id))
            continue;

        const int row = checkboxIndex / 2;
        const int column = checkboxIndex % 2;

        button.x = column == 0 ? BUTTON_X : PANEL_W / 2.0f;
        button.y = checkboxesY + row * CHECKBOX_ROW_SPACING;

        checkboxIndex++;
    }

    // Section 4: sliders
    const int checkboxRows = (checkboxIndex + 1) / 2;
    const float slidersY = checkboxesY + checkboxRows * CHECKBOX_ROW_SPACING + SECTION_SPACING;

    i = 0;
    for (Slider &slider : _sliders)
    {
        // Normalize the initial slider value
        slider.value = normalizeValue(slider.initialValue, slider.min, slider.max);

        // Set the slider Y position
        slider.y = slidersY + i * SLIDER_ROW_SPACING;
        i++;
    }

    // Create the panel window
    window = SDL_CreateWindow("Controls", PANEL_W, panelHeight(), SDL_WINDOW_RESIZABLE);

    // Create the renderer for panel
    r = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // Get window ID
    windowID = SDL_GetWindowID(window);

    // Set the window position
    SDL_SetWindowPosition(window, static_cast<int>(sw - PANEL_W), 40);
}

void Panel::handleEvent(const SDL_Event &event)
{
    // Ignore events if the panel is closed or the event is from another window
    if (!window || eventWindowID(event) != windowID)
        return;

    // Close the panel if the close button is pressed
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        close();
        return;
    }

    // Stop editing when the panel loses keyboard focus
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST)
    {
        endEditing(false);
        return;
    }

    if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
    {
        if (_editingIndex >= 0)
            SDL_StartTextInput(window);
        return;
    }

    // Typing into a value box, or keys used to edit it
    if (_editingIndex >= 0 && handleTextBoxInput(event))
        return;

    // Handle button clicks and value box clicks
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
    {
        if (handleTextBoxClick(event))
            return;

        endEditing(true);
        handleButtonEvent(event);
    }

    // While the simulation is running the sliders are locked (only Reset re-enables
    // them): clicks, knobs and drags on the slider section are all ignored.
    // if (buttonValue("start"))
    //     return;

    // Handle each slider
    for (Slider &slider : _sliders)
    {
        const bool locked = sliderLocked(slider);

        // A locked slider must never keep a drag alive
        if (locked)
            slider.dragging = false;

        // Check if the left mouse button is pressed
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            if (!locked)
                handleKnobEvent(slider, event);
        }
        // Check if the left mouse button is released
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
            slider.dragging = false;
        // Move the slider if the knob is being dragged
        else if (event.type == SDL_EVENT_MOUSE_MOTION && slider.dragging)
            handleSliderValue(slider, event);
    }
}

bool Panel::sliderLocked(const Slider &slider) const
{
    return slider.blockDuringSim && buttonValue("start");
}

bool Panel::handleTextBoxClick(const SDL_Event &event)
{
    for (size_t i = 0; i < _sliders.size(); i++)
    {
        if (sliderLocked(_sliders[i]))
            continue;

        if (!isMouseInRect(valueBoxRect(_sliders[i]), event.button.x, event.button.y))
            continue;

        // Committing the previous box first keeps only one edit alive at a time
        const int index = static_cast<int>(i);
        if (_editingIndex >= 0 && _editingIndex != index)
            endEditing(true);

        beginEditing(index);
        return true;
    }

    return false;
}

bool Panel::handleTextBoxInput(const SDL_Event &event)
{
    Slider &slider = _sliders[_editingIndex];

    if (event.type == SDL_EVENT_TEXT_INPUT)
    {
        if (slider.selectAll)
        {
            slider.textBuffer.clear();
            slider.selectAll = false;
        }

        for (const char *c = event.text.text; *c; c++)
        {
            const bool allowed = (*c >= '0' && *c <= '9') || *c == '.' || *c == '-';
            if (allowed && slider.textBuffer.size() < 12)
                slider.textBuffer.push_back(*c);
        }

        return true;
    }

    if (event.type != SDL_EVENT_KEY_DOWN)
        return false;

    switch (event.key.scancode)
    {
    case SDL_SCANCODE_BACKSPACE:
        if (slider.selectAll)
        {
            slider.textBuffer.clear();
            slider.selectAll = false;
        }
        else if (!slider.textBuffer.empty())
            slider.textBuffer.pop_back();
        break;

    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
        endEditing(true);
        return true;

    case SDL_SCANCODE_ESCAPE:
        endEditing(false);
        return true;

    case SDL_SCANCODE_TAB:
    {
        const int current = _editingIndex;
        const int step = (event.key.mod & SDL_KMOD_SHIFT) ? -1 : 1;
        endEditing(true);

        for (int next = current + step; next >= 0 && next < static_cast<int>(_sliders.size()); next += step)
        {
            if (sliderLocked(_sliders[next]))
                continue;

            beginEditing(next);
            break;
        }
        return true;
    }

    default:
        return true;
    }

    return true;
}

void Panel::beginEditing(const int index)
{
    if (index < 0 || index >= static_cast<int>(_sliders.size()))
        return;

    if (_editingIndex == index || sliderLocked(_sliders[index]))
        return;

    Slider &slider = _sliders[index];
    slider.textBuffer = sliderValueText(value(slider.id));
    slider.selectAll = true;
    slider.editing = true;
    _editingIndex = index;

    SDL_StartTextInput(window);
}

void Panel::endEditing(const bool commit)
{
    if (_editingIndex < 0)
        return;

    if (commit)
        commitEditing();

    _sliders[_editingIndex].editing = false;
    _sliders[_editingIndex].selectAll = false;
    _sliders[_editingIndex].textBuffer.clear();
    _editingIndex = -1;

    SDL_StopTextInput(window);
}

void Panel::commitEditing()
{
    if (_editingIndex < 0)
        return;

    Slider &slider = _sliders[_editingIndex];

    float typed = 0.0f;
    if (!parseNumber(slider.textBuffer, typed))
        return;

    typed = std::clamp(typed, slider.min, slider.max);

    const float previous = value(slider.id);
    slider.value = normalizeValue(typed, slider.min, slider.max);

    if (slider.listener && value(slider.id) != previous)
        slider.changed = true;
}

bool Panel::parseNumber(const std::string &text, float &result)
{
    if (text.empty())
        return false;

    char *end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (end != text.c_str() + text.size())
        return false;

    result = static_cast<float>(parsed);
    return std::isfinite(result);
}

bool Panel::isEditing() const
{
    return _editingIndex >= 0;
}

void Panel::render(const float mouseX, const float mouseY) const
{
    if (!window || !r)
        return;

    // Clear the panel
    Colors::VeryDarkGray(r);
    SDL_RenderClear(r);

    // Draw text lines
    for (const PanelText &text : _texts)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << text.value;
        drawText(r, text.label + ss.str(), TEXT_X, text.y, 14);
    }

    // Draw each button
    int checkboxIndex = 0;
    for (const Button &button : _buttons)
    {
        if (button.id == "start" || button.id == "reset" || button.id == "pause")
        {
            const SDL_FRect buttonRect = {
                button.x, button.y,
                actionButtonWidth(), BUTTON_H};

            SDL_Color textColor = {255, 255, 255, 255};

            if (!button.visibility)
            {
                Colors::DarkGray(r);
                textColor = {200, 200, 200, 255};
            }
            else if (button.value)
            {
                Colors::Green(r);
                textColor = {0, 0, 0, 255};
            }
            else
                Colors::Gray(r);

            SDL_RenderFillRect(r, &buttonRect);

            // Set button text
            std::string text = button.text;
            if (button.id == "pause")
                text = button.value ? "Resume" : "Pause";

            // Draw button text
            drawText(r, text, buttonRect.x + 12.0f, buttonRect.y + 4.0f, 16, textColor);

            // Hover effect
            if (isMouseInRect(buttonRect, mouseX, mouseY) && button.visibility)
            {
                SDL_SetRenderDrawColor(r, 0, 0, 0, 50);
                SDL_RenderFillRect(r, &buttonRect);
            }
        }
        else
        {
            const int row = checkboxIndex / 2;
            const int column = checkboxIndex % 2;
            const float x = column == 0 ? BUTTON_X : PANEL_W / 2.0f;

            const SDL_FRect checkbox = {x, button.y, CHECKBOX_SIZE, CHECKBOX_SIZE};

            SDL_Color textColor = {255, 255, 255, 255};

            if (!button.visibility)
            {
                Colors::DarkGray(r);
                textColor = {200, 200, 200, 255};
            }
            else if (button.value)
            {
                Colors::Green(r);
                textColor = {0, 255, 0, 255};
            }
            else
                Colors::Gray(r);

            SDL_RenderFillRect(r, &checkbox);

            // Draw button text
            std::string text = button.text;
            if (button.id == "pause")
                text = button.value ? "Resume" : "Pause";

            drawText(r, text, checkbox.x + checkbox.w + 6.0f, checkbox.y - 2.0f, 14, textColor);

            if (isMouseInRect(checkbox, mouseX, mouseY) && button.visibility)
            {
                SDL_SetRenderDrawColor(r, 0, 0, 0, 50);
                SDL_RenderFillRect(r, &checkbox);
            }

            checkboxIndex++;
        }
    }

    // Draw each slider
    for (const Slider &s : _sliders)
    {
        const bool locked = sliderLocked(s);

        // Draw the slider track
        const SDL_FRect trackRect = {
            SLIDER_X, s.y + SLIDER_H / 3.0f,
            trackWidth(), SLIDER_H / 3.0f};

        Colors::MediumGray(r);
        SDL_RenderFillRect(r, &trackRect);

        // Draw the slider knob. While the simulation is running the locked
        // sliders are drawn dimmed to show they can't be changed.
        const SDL_FRect knob = knobRect(s);

        if (locked)
        {
            Colors::DarkGray(r);
            SDL_RenderFillRect(r, &knob);
        }
        else
        {
            Colors::MustardYellow(r);
            SDL_RenderFillRect(r, &knob);
        }

        // Draw the slider label
        drawText(r, s.id, LABEL_X, s.y, 14);

        // Draw the inline value box
        const SDL_FRect box = valueBoxRect(s);
        const std::string text = s.editing
                                     ? s.textBuffer
                                     : sliderValueText(value(s.id));

        Colors::VeryDarkGray(r);
        SDL_RenderFillRect(r, &box);

        const bool hovered = isMouseInRect(box, mouseX, mouseY);

        if (locked)
            Colors::DarkGray(r);
        else if (s.editing)
            Colors::MustardYellow(r);
        else if (hovered)
            Colors::White(r);
        else
            Colors::MediumGray(r);

        SDL_RenderRect(r, &box);

        const float textX = box.x + VALUE_BOX_PADDING;
        const float textY = box.y + (box.h - VALUE_FONT_SIZE) / 2.0f - 1.0f;
        drawText(r, text, textX, textY, VALUE_FONT_SIZE);

        // Caret at the end of the edited text (blinks on the panel refresh rate)
        if (s.editing && ((SDL_GetTicks() / 500) % 2 == 0))
        {
            int textW = 0, textH = 0;
            if (TTF_GetStringSize(cachedFont(VALUE_FONT_SIZE), text.c_str(), text.size(), &textW, &textH))
            {
                const SDL_FRect caret = {
                    textX + static_cast<float>(textW) + 1.0f,
                    box.y + 4.0f,
                    1.0f,
                    box.h - 8.0f};

                Colors::White(r);
                SDL_RenderFillRect(r, &caret);
            }
        }
    }

    // Show the rendered panel
    SDL_RenderPresent(r);
}

float Panel::value(const std::string &id) const
{
    for (const Slider &slider : _sliders)
    {
        if (slider.id == id)
        {
            // Convert the slider value to its actual value
            return slider.min + slider.value * (slider.max - slider.min);
        }
    }

    return 0.0f;
}

bool Panel::buttonValue(const std::string &id) const
{
    for (const Button &button : _buttons)
    {
        if (button.id == id)
            return button.value;
    }

    return false;
}

void Panel::setText(const std::string &id, float value)
{
    for (PanelText &text : _texts)
    {
        if (text.id == id)
        {
            text.value = value;
            return;
        }
    }
}

void Panel::close()
{
    endEditing(false);

    Window::close();

    for (Slider &slider : _sliders)
    {
        slider.dragging = false;
        slider.changed = false;
    }

    for (Button &button : _buttons)
        button.changed = false;
}

SDL_FRect Panel::knobRect(const Slider &slider)
{
    // Calculate the knob position and size
    return {
        SLIDER_X + slider.value * (trackWidth() - SLIDER_H),
        slider.y,
        SLIDER_H,
        SLIDER_H};
}

SDL_FRect Panel::valueBoxRect(const Slider &slider)
{
    return {
        SLIDER_X + trackWidth() + VALUE_BOX_GAP,
        slider.y,
        VALUE_BOX_W,
        SLIDER_H};
}

float Panel::panelHeight() const
{
    // Use the default height if there are no controls
    if (_texts.empty() && _sliders.empty() && _buttons.empty())
        return PANEL_MARGIN * 2.0f;

    const float textsBottom = TEXT_TOP + static_cast<float>(_texts.size()) * TEXT_SPACING;
    const float buttonsY = textsBottom + SECTION_SPACING;
    const float checkboxesY = buttonsY + BUTTON_H + SECTION_SPACING;

    const int checkboxCount = static_cast<int>(_buttons.size()) - actionButtonCount();
    const int checkboxRows = (checkboxCount + 1) / 2;

    const float slidersY = checkboxesY + checkboxRows * CHECKBOX_ROW_SPACING + SECTION_SPACING;

    // Return the top of the slider section if there are no sliders
    if (_sliders.empty())
        return slidersY + SLIDER_H + PANEL_BOTTOM;

    return slidersY +
           static_cast<float>(_sliders.size() - 1) * SLIDER_ROW_SPACING +
           SLIDER_H + PANEL_BOTTOM;
}

float Panel::sliderWidth()
{
    return PANEL_W - SLIDER_X - PANEL_MARGIN;
}

float Panel::trackWidth()
{
    return sliderWidth() - VALUE_BOX_W - VALUE_BOX_GAP;
}

float Panel::actionButtonWidth()
{
    return (PANEL_W - PANEL_MARGIN * 2.0f - BUTTON_GAP * 2.0f) / 3.0f;
}

int Panel::actionButtonCount() const
{
    int count = 0;

    for (const Button &button : _buttons)
    {
        if (isActionButton(button.id))
            count++;
    }

    return count;
}

bool Panel::isActionButton(const std::string &id)
{
    return id == "start" || id == "reset" || id == "pause";
}

float Panel::normalizeValue(float value, float min, float max)
{
    const float range = max - min;
    if (range <= 0.0f)
        return 0.0f;

    // Convert the value to a range between 0 and 1
    const float normalized = (value - min) / range;

    return clamp01(normalized);
}

float Panel::clamp01(float value)
{
    if (value < 0.0f)
        return 0.0f;

    if (value > 1.0f)
        return 1.0f;

    return value;
}

void Panel::handleSliderValue(Slider &slider, const SDL_Event &event)
{
    // Calculate the slider value from the mouse position
    const float oldValue = slider.value;

    slider.value = (event.motion.x - SLIDER_X) / (trackWidth() - SLIDER_H);
    slider.value = clamp01(slider.value);

    // Check if the slider value changed
    if (slider.listener && slider.value != oldValue)
        slider.changed = true;
}

void Panel::handleKnobEvent(Slider &slider, const SDL_Event &event)
{
    // Get the knob position
    const SDL_FRect knob = knobRect(slider);

    // Check if the mouse is inside the knob
    slider.dragging = isMouseInRect(
        knob,
        event.button.x,
        event.button.y);
}

void Panel::handleButtonEvent(const SDL_Event &event)
{
    int checkboxIndex = 0;

    for (Button &button : _buttons)
    {
        if (!button.visibility)
            continue;

        SDL_FRect buttonRect;

        if (button.id == "start" || button.id == "reset" || button.id == "pause")
        {
            buttonRect = {
                button.x, button.y,
                actionButtonWidth(), BUTTON_H};
        }
        else
        {
            const int column = checkboxIndex % 2;
            const float x = column == 0 ? BUTTON_X : PANEL_W / 2.0f;

            buttonRect = {x, button.y, CHECKBOX_SIZE, CHECKBOX_SIZE};
            checkboxIndex++;
        }

        if (isMouseInRect(buttonRect, event.button.x, event.button.y))
        {
            if (button.id == "start")
            {
                button.value = true;
                button.changed = button.listener;

                setVisibility("start", false);
                setVisibility("reset", true);
                setVisibility("pause", true);
            }
            else if (button.id == "reset")
            {
                button.value = false;
                button.changed = button.listener;

                setValue("start", false);
                setValue("pause", false);

                setVisibility("start", true);
                setVisibility("reset", false);
                setVisibility("pause", false);
            }
            else
            {
                button.value = !button.value;
                button.changed = button.listener;
            }

            return;
        }
    }
}

bool Panel::isMouseInRect(const SDL_FRect &rect, const float mouseX, const float mouseY)
{
    return mouseX >= rect.x && mouseX <= rect.x + rect.w &&
           mouseY >= rect.y && mouseY <= rect.y + rect.h;
}

void Panel::setValue(const std::string &id, bool value)
{
    for (Button &button : _buttons)
    {
        if (button.id == id)
        {
            if (button.value != value && button.listener)
                button.changed = true;

            button.value = value;
            break;
        }
    }
}

void Panel::setVisibility(const std::string &id, bool visibility)
{
    for (Button &button : _buttons)
    {
        if (button.id == id)
        {
            button.visibility = visibility;
            break;
        }
    }
}

bool Panel::btnListener(const std::string &id)
{
    for (Button &button : _buttons)
    {
        if (button.id == id)
        {
            if (button.listener && button.changed)
            {
                button.changed = false;
                return true;
            }

            return false;
        }
    }

    return false;
}

bool Panel::sliderListener(const std::string &id)
{
    for (Slider &slider : _sliders)
    {
        if (slider.id == id)
        {
            if (slider.listener && slider.changed)
            {
                slider.changed = false;
                return true;
            }

            return false;
        }
    }

    return false;
}