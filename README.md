# Simple Pyshics Enginee

A small C++ physics sandbox built with C++ and SDL3.

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

4. Update the SDL paths in `build.bat` if they are different on your computer.
5. Build the project:

```bash
build.bat
```

6. Run the application:

```bash
main.exe
```

## First Step

The first thing we did is to create a circle using triangles, so we use SDL_RenderGeometry() for it.

![First shape-generation test](images/step1.png)

## Second Step: Gravity

The second step adds gravity so particles accelerate and fall over time.

![Gravity simulation](images/gravity.gif)

## Third Step: Collision

The third step adds collisions between particles and the screen boundaries. The current step can run up to only a *1,000* particles at 60 FPS.

This step collision method checks every particle against every other particle to find collisions. It also checks every particle against the screen boundaries on each update.

![Collision simulation](images/collision.gif)

And this is the third step collision equations:

![Collision equations](images/collisionExplaination.png)

## Fourth Step: Gravitational Force

In this step, gravitational force was added to simulate the orbit of the Moon around the Earth.

The gravitational acceleration acting on object **A** due to object **B** is calculated using Newton's law of universal gravitation:

$$ a = \\frac{Gm_b}{r^2} $$

Where:

- (a) = gravitational acceleration acting on object A
- (G) = gravitational constant used in the simulation
- (m_b) = mass of object B
- (r) = distance between the two objects

```cpp
float acc = (G * m_b) / (r * r);
```

The acceleration is then applied in the direction of object B, causing the Moon to orbit the Earth.

![Earth and moon simulation](images/earthAndMoongif.gif)

## Grid Visualization

To understand how spatial partitioning works, we visualized which cells particles occupy. As particles enter cells, they light them up.

![Grid visualization](images/grid.gif)

## Fifth Step: Spatial Hash

Checking every particle against every other particle does not scale, so before the fluid is computed the particles are sorted into a grid. The cell size is the interaction radius, so a particle only has to look at its own cell and the 8 cells around it to find every neighbour that can affect it.

Each cell is stored in a hash map, so the grid does not have to be allocated for the whole world, and the set of neighbouring particles is found in a few steps instead of a full scan.

```cpp
uint32_t hash = (cellX * prime1 ^ cellY * prime2) % hashMapSize;
```

![Spatial hash](images/hash.gif)

## Sixth Step: Double Density Relaxation

The last step makes the particles behave like a fluid. It uses the double density relaxation method from Clavet et al. 2005, where every particle looks at the neighbours found by the hash and measures two densities, with `q` being the distance to the neighbour divided by the interaction radius `h`:

- `density = Σ(1 - q)²`
- `densityNear = Σ(1 - q)³`

The two densities are turned into pressures:

```cpp
float pressure = Globals::k * (density - Globals::targetDensity);
float pressureNear = Globals::kNear * densityNear;
```

and then every pair of particles is pushed apart by:

```cpp
float displacementTerm = dt * dt * (pressure * (1 - q) + pressureNear * (1 - q) * (1 - q));
```

`targetDensity` is the density the fluid relaxes towards: above it the fluid contracts, below it the fluid expands. `k` controls how stiff the fluid is and `kNear` keeps the closest particles from collapsing into each other.

![Fluid simulation](images/fluid.gif)

## Seventh Step: Viscosity

The density pass makes the particles behave like a fluid, but on its own it is almost frictionless. Viscosity adds a damping impulse between particles that are moving towards each other, using the same neighbours found by the hash. With `q` being the distance divided by the interaction radius and `u` the closing speed along the line between the two particles:

$$ I = dt(1 - q)(\sigma u + \beta u^2) $$

```cpp
float u = Dot(Sub(particleA.velocity, particleB.velocity), rij);
float ITerm = dt * (1.0f - q) * (Globals::Sigma * u + Globals::Beta * u * u);
```

`Sigma` is the linear term and `Beta` the quadratic one, so `Beta` dominates at high closing speeds. Both are multiplied by `dt`, which is why they have to be large to be noticeable: a value near `0` is water, a few hundred is syrup or honey, and thousands turn the fluid into a near-solid.

![Honey simulation (bottom-left of the four simulations)](images/springsGE.gif)

## Eighth Step: Springs

The last pass adds the elastic part from Clavet et al. 2005. Every pair of particles closer than the interaction radius gets a spring whose rest length starts at `h`, and every spring pulls its two ends towards that rest length. Plasticity lets the rest length follow the actual distance, with a dead zone of `gamma` times the rest length:

```cpp
float d = Globals::gamma * Lij;

if (rij > Lij + d)
    spring.restLength += dt * Globals::plasticity * (rij - Lij - d);
else if (rij < Lij - d)
    spring.restLength -= dt * Globals::plasticity * (Lij - d - rij);
```

The springs are then solved by moving each pair towards its rest length:

```cpp
float displacementTerm = dt * dt * Globals::springStiffness *
                         (1.0f - spring.restLength / Globals::interatctionRadius) *
                         (spring.restLength - distance);
```

`gamma` controls how much drift the spring ignores, `plasticity` how fast it forgets its original length (small keeps the elastic memory, large keeps the deformed shape), and `springStiffness` how hard it corrects. Because the displacement is scaled by `dt * dt`, stiffness has to be in the thousands to be visible at a fixed step of `1/120` s.

![Springs simulation](images/springsGE.gif)

## Libraries

- [SDL3](https://github.com/libsdl-org/SDL): window creation, rendering, input, and geometry drawing.
- [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf): TrueType font support for on-screen text.