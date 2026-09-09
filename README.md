# Pyshics

A small C++ physics sandbox built with C++ and SDL3.

## First Step

The first thing we did is to create a circle using triangles, so we use SDL_RenderGeometry() for it.

![First shape-generation test](images/step1.png)

## Second Step: Gravity

The second step adds gravity so particles accelerate and fall over time.

![Gravity simulation](images/gravity.gif)

## Third Step: Collision

The third step adds collisions between particles and the screen boundaries.

![Collision simulation](images/collision.gif)

## Libraries

- [SDL3](https://github.com/libsdl-org/SDL): window creation, rendering, input, and geometry drawing.
- [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf): TrueType font support for on-screen te
