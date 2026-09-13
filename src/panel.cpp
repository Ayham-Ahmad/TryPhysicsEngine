#include "panel.h"

Panel::~Panel()
{
    close();
}

void Panel::init(float sw)
{
    // Initialize the sliders: {id, initValue, min, max, listener}
    _sliders = {
        {"speed", 1.0f, 0.1f, 10.0f, false},
        {"gravity", 981.0f, 0.0f, 5000.0f, false},
        {"count", 100.0f, 1.0f, 2000.0f, true},
        {"spacing", 25.0f, 5.0f, 200.0f, true},
        {"size", 10.0f, 1.0f, 50.0f, true},
        {"gridSize", 100.0f, 1.0f, 250.0f, true},
        {"smoothingRadius", 20, 1, 200, true},
    };

    // Initialize the buttons: {id, text, value, visibility, listener}
    _buttons = {
        {"start", "Start", false, true},
        {"reset", "Reset", false, false, true},
        {"pause", "Pause", false, false},
        {"grid", "Grid", false, true},
        {"graph", "Graph", false, true},
        {"gravity", "Gravity", true, true},
        {"gforce", "GForce", true, true},
        {"screenCollision", "Screen Collision", true, true},
        {"particles", "Show Particles", true, true},
        {"random", "random", false, true, true},
    };

    // Set PositionY for each button
    int i = 0;
    for (Button &button : _buttons)
    {
        if (button.id == "start" || button.id == "reset" || button.id == "pause")
        {
            button.y = SLIDER_TOP + i * SLIDER_SPACING;
            i++;
        }
    }

    // Set PositionY for each checkbox button
    int checkboxIndex = 0;
    for (Button &button : _buttons)
    {
        if (button.id == "start" || button.id == "reset" || button.id == "pause")
            continue;

        const int row = checkboxIndex / 2;
        const int column = checkboxIndex % 2;

        button.y = SLIDER_TOP + 2 * SLIDER_SPACING + row * SLIDER_SPACING + SECTIONS_SPACING;
        button.x = column == 0 ? BUTTON_X : PANEL_W / 2.0f;

        checkboxIndex++;
    }

    // Set PositionY for each slider
    const int checkboxRows = (checkboxIndex + 1) / 2;
    const float sliderTop = SLIDER_TOP + 2 * SLIDER_SPACING +
                            checkboxRows * SLIDER_SPACING + BUTTON_TOP_MARGIN;

    i = 0;
    for (Slider &slider : _sliders)
    {
        // Normalize the initial slider value
        slider.value = normalizeValue(slider.initialValue, slider.min, slider.max);

        // Set the slider Y position
        slider.y = sliderTop + i * SLIDER_SPACING * 0.75f + SECTIONS_SPACING;
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

    // Handle button clicks
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
        handleButtonEvent(event);

    // Handle each slider
    for (Slider &slider : _sliders)
    {
        // Check if the left mouse button is pressed
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
            handleKnobEvent(slider, event);
        // Check if the left mouse button is released
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
            slider.dragging = false;
        // Move the slider if the knob is being dragged
        else if (event.type == SDL_EVENT_MOUSE_MOTION && slider.dragging)
            handleSliderValue(slider, event);
    }
}

void Panel::render(const float mouseX, const float mouseY)
{
    // Clear the panel
    c.VeryDarkGray(r);
    SDL_RenderClear(r);

    // Draw each button
    int checkboxIndex = 0;
    for (const Button &button : _buttons)
    {
        if (button.id == "start" || button.id == "reset" || button.id == "pause")
        {
            const SDL_FRect buttonRect = {
                BUTTON_X, button.y,
                PANEL_W - BUTTON_X * 2.0f, BUTTON_H};

            SDL_Color textColor = {255, 255, 255, 255};

            if (!button.visibility)
            {
                c.DarkGray(r);
                textColor = {200, 200, 200, 255};
            }
            else if (button.value)
            {
                c.Green(r);
                textColor = {0, 0, 0, 255};
            }
            else
                c.Gray(r);

            SDL_RenderFillRect(r, &buttonRect);

            // Set button text
            std::string text = button.text;
            if (button.id == "pause")
                text = button.value ? "Pause" : "Resume";

            // Draw button text
            drawText(r, text, BUTTON_X + 20.0f, buttonRect.y + 4.0f, 16, textColor);

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

            const SDL_FRect checkbox = {x, button.y, 16.0f, 16.0f};

            SDL_Color textColor = {255, 255, 255, 255};

            if (!button.visibility)
            {
                c.DarkGray(r);
                textColor = {200, 200, 200, 255};
            }
            else if (button.value)
            {
                c.Green(r);
                textColor = {0, 255, 0, 255};
            }
            else
                c.Gray(r);

            SDL_RenderFillRect(r, &checkbox);

            // Draw button text
            std::string text = button.text;
            if (button.id == "pause")
                text = button.value ? "Pause" : "Resume";

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
        // Draw the slider track
        const SDL_FRect trackRect = {
            SLIDER_X, s.y + SLIDER_H / 3.0f,
            sliderWidth(), SLIDER_H / 3.0f};

        c.MediumGray(r);
        SDL_RenderFillRect(r, &trackRect);

        // Draw the slider knob
        const SDL_FRect knob = knobRect(s);

        c.MustardYellow(r);
        SDL_RenderFillRect(r, &knob);

        // Draw the slider label and value
        drawText(r, s.id, LABEL_X, s.y, 14);
        drawText(r, (int)value(s.id), knob.x, knob.y - 14.0f, 10);
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

void Panel::close()
{
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

    for (Slider &slider : _sliders)
    {
        slider.dragging = false;
        slider.changed = false;
    }

    for (Button &button : _buttons)
        button.changed = false;
}

SDL_FRect Panel::knobRect(const Slider &slider) const
{
    // Calculate the knob position and size
    return {
        SLIDER_X + slider.value * (sliderWidth() - SLIDER_H),
        slider.y,
        SLIDER_H,
        SLIDER_H};
}

float Panel::panelHeight() const
{
    // Use the default height if there are no controls
    if (_sliders.empty() && _buttons.empty())
        return SLIDER_TOP + PANEL_BOTTOM;

    const int checkboxCount = static_cast<int>(_buttons.size()) - 3;
    const int checkboxRows = (checkboxCount + 1) / 2;

    const float sliderTop = SLIDER_TOP + 2 * SLIDER_SPACING +
                            checkboxRows * SLIDER_SPACING +
                            BUTTON_TOP_MARGIN + SECTIONS_SPACING;

    return sliderTop +
           static_cast<float>(_sliders.size() - 1) * SLIDER_SPACING * 0.75f +
           SLIDER_H + PANEL_BOTTOM;
}

float Panel::sliderWidth() const
{
    return PANEL_W - SLIDER_X - PANEL_MARGIN;
}

float Panel::normalizeValue(float value, float min, float max)
{
    const float range = max - min;
    if (range <= 0.0f)
        return 0.0f;

    // Convert the value to a range between 0 and 1
    const float normalized = (value - min) / range;

    return keepbetween0and1(normalized);
}

float Panel::keepbetween0and1(float value)
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

    slider.value = (event.motion.x - SLIDER_X) / (sliderWidth() - SLIDER_H);
    slider.value = keepbetween0and1(slider.value);

    // Check if the slider value changed
    if (slider.listener && slider.value != oldValue)
        slider.changed = true;
}

void Panel::handleKnobEvent(Slider &slider, const SDL_Event &event) const
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
                BUTTON_X, button.y,
                PANEL_W - BUTTON_X * 2.0f, BUTTON_H};
        }
        else
        {
            const int column = checkboxIndex % 2;
            const float x = column == 0 ? BUTTON_X : PANEL_W / 2.0f;

            buttonRect = {x, button.y, 16.0f, 16.0f};
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

bool Panel::isMouseInRect(const SDL_FRect &rect, const float mouseX, const float mouseY) const
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

SDL_WindowID Panel::eventWindowID(const SDL_Event &event) const
{
    // Get window ID
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