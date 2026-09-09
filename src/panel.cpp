#include "panel.h"

Panel::~Panel()
{
    close();
}

void Panel::init(float sw)
{
    // Initialize the sliders: {id, initValue, min, max}
    sliders = {
        {"gravity", GRAVITY, 0.0f, 5000.0f},
        // {"accelX", 0.0f, 0.0f, 100.0f},
        {"resolution", 100.0f, 3.0f, 100.0f}};

    // Set PositionY for each slider
    int i = 0;
    for (Slider &slider : sliders)
    {
        // Normalize the initial slider value
        slider.value = normalizeValue(slider.initialValue, slider.min, slider.max);

        // Set the slider Y position
        slider.y = SLIDER_TOP + i * SLIDER_SPACING;
        i++;
    }

    // Create the panel window
    window = SDL_CreateWindow("Controls", PANEL_W, panelHeight(), SDL_WINDOW_RESIZABLE);

    // Create the renderer for panel
    r = SDL_CreateRenderer(window, nullptr);

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

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        event.button.button == SDL_BUTTON_LEFT)
    {
        handlePauseEvent(event);
    }

    // Handle each slider
    for (Slider &slider : sliders)
    {
        // Check if the left mouse button is pressed
        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            event.button.button == SDL_BUTTON_LEFT)
        {
            handleKnobEvent(slider, event);
        }
        // Check if the left mouse button is released
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                 event.button.button == SDL_BUTTON_LEFT)
        {
            slider.dragging = false;
        }
        // Move the slider if the knob is being dragged
        else if (event.type == SDL_EVENT_MOUSE_MOTION && slider.dragging)
        {
            handleSliderValue(slider, event);
        }
    }
}

void Panel::render()
{
    // Clear the panel
    c.VeryDarkGray(r);
    SDL_RenderClear(r);

    // Draw each slider
    c.MediumGray(r);
    for (const Slider &s : sliders)
    {
        // Draw the slider track
        const SDL_FRect trackRect = {SLIDER_X, s.y + SLIDER_H / 3.0f, sliderWidth(), SLIDER_H / 3.0f};
        SDL_RenderFillRect(r, &trackRect);

        // Draw the slider knob
        c.MustardYellow(r);
        const SDL_FRect knob = knobRect(s);
        SDL_RenderFillRect(r, &knob);

        // Draw the slider label and value
        drawText(r, s.id, LABEL_X, s.y, 16);
        drawText(r, (int)value(s.id), knob.x, knob.y - 16.0f, 12);

        c.MediumGray(r);
    }

    // Set the pause button position and size
    const SDL_FRect buttonRect = {
        BUTTON_X,
        panelHeight() - PANEL_BOTTOM - BUTTON_H,
        PANEL_W - BUTTON_X * 2.0f,
        BUTTON_H};

    // Draw the pause button
    c.DarkGray(r);
    SDL_RenderFillRect(r, &buttonRect);

    // Draw the pause button text
    drawText(
        r,
        isPaused ? "Resume" : "Pause",
        BUTTON_X + 8.0f,
        buttonRect.y + 4.0f,
        16);

    // Show the rendered panel
    SDL_RenderPresent(r);
}

float Panel::value(const std::string &id) const
{
    for (const Slider &slider : sliders)
    {
        if (slider.id == id)
        {
            // Convert the slider value to its actual value
            return slider.min + slider.value * (slider.max - slider.min);
        }
    }

    return 0.0f;
}

bool Panel::paused() const
{
    return isPaused;
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

    for (Slider &slider : sliders)
        slider.dragging = false;
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
    // Use the default height if there are no sliders
    if (sliders.empty())
        return SLIDER_TOP + PANEL_BOTTOM;

    // Calculate the panel height from the number of sliders
    return SLIDER_TOP +
           (sliders.size() - 1) * SLIDER_SPACING +
           SLIDER_H + BUTTON_TOP_MARGIN +
           BUTTON_H + PANEL_BOTTOM;
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
    else if (value > 1.0f)
        return 1.0f;

    return value;
}

void Panel::handleSliderValue(Slider &slider, const SDL_Event &event) const
{
    // Calculate the slider value from the mouse position
    slider.value =
        (event.motion.x - SLIDER_X) /
        (sliderWidth() - SLIDER_H);

    slider.value = keepbetween0and1(slider.value);
}

void Panel::handleKnobEvent(Slider &slider, const SDL_Event &event) const
{
    // Get the knob position
    const SDL_FRect knob = knobRect(slider);

    // Check if the mouse is inside the knob
    if (event.button.x >= knob.x &&
        event.button.x <= knob.x + knob.w &&
        event.button.y >= knob.y &&
        event.button.y <= knob.y + knob.h)
    {
        slider.dragging = true;
    }
}

void Panel::handlePauseEvent(const SDL_Event &event)
{
    // Set the pause button position and size
    const SDL_FRect buttonRect = {
        BUTTON_X,
        panelHeight() - PANEL_BOTTOM - BUTTON_H,
        PANEL_W - BUTTON_X * 2.0f,
        BUTTON_H};

    // Check if the mouse is inside the button
    if (event.button.x >= buttonRect.x &&
        event.button.x <= buttonRect.x + buttonRect.w &&
        event.button.y >= buttonRect.y &&
        event.button.y <= buttonRect.y + buttonRect.h)
    {
        // Toggle the pause state
        isPaused = !isPaused;
        return;
    }
}

SDL_WindowID Panel::eventWindowID(const SDL_Event &event) const
{
    // Get the window ID from the event
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