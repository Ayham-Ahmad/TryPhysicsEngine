# Simple Physics Engine

A small 2D fluid-physics sandbox built with C++ and SDL3.

## Setup and Run

1. Install MinGW with `g++` and make sure it is available in your PATH.
2. Download and install the SDL3 development library and the SDL3_ttf development library:
	- [SDL3](https://github.com/libsdl-org/SDL/releases)
	- [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf/releases)
3. Clone the repository:

```bash
git clone https://github.com/Ayham-Ahmad/TryPhysicsEngine.git
cd TryPhysicsEngine
```

4. Update the SDL include paths in `Makefile` if they are different on your computer.
5. Build the project with `make` (or run `build.bat` which wraps it):

```bash
make
```

6. Run the application:

```bash
main.exe
```

## Control Panel

A second window lets you tune the simulation live:

- **Buttons** (toggle checkboxes): draw the occupancy grid, open the metrics graph,
  enable gravity, screen-boundary handling, particles, the soft glow circles,
  gradient arrows, and speed-based colouring.
- **Start / Pause / Reset** control the simulation. While running, the sliders are
  locked so the fluid stays consistent; press Reset to apply new values.
- **Sliders**: particle count, spacing, size, grid size, smoothing radius, particle
  mass, target density, pressure multiplier, and simulation speed.

## Smooth-Particle Hydrodynamics

The fluid is simulated with SPH, ported line-for-line from Sebastian Lague's
*"Coding Adventure: Simulating Fluids"* video:

- **Density** — each particle's density is accumulated from its neighbours using a
  kernel that falls to zero at the smoothing radius:

```cpp
density += mass * (radius - dist)² * 6 / (π · radius⁴)
```

- **Pressure force** — density error against the rest density is converted to a
  pressure, then shared between neighbours. The pressure-gradient force pushes
  compressed particles apart:

```cpp
densityError = density - targetDensity;
pressure = clamp(densityError * pressureMultiplier, 0, ∞);
```

- A neighbour-search **spatial hash** keeps the density/pressure sweeps O(n · k)
  instead of O(n²), so the sim stays near real-time up to ~1000+ particles.

## Rendering

- Particles (instanced polygons via `SDL_RenderGeometry`) with either speed-based
  or pressure-based colouring.
- Soft additive glow around each particle, batched into a single textured geometry
  draw (one draw call for the whole field).
- Optional gradient-arrow field that samples the pressure gradient once per frame.
- Metrics graph (entropy, average speed, kinetic energy, acceleration, density) with
  a ring-buffer history.

## Libraries

- [SDL3](https://github.com/libsdl-org/SDL): window creation, rendering, input, and geometry drawing.
- [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf): TrueType font support for on-screen text.