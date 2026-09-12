#pragma once

class Globals
{
public:
    inline static float pixelsPerMeter = 20.0f;
    inline static float gravity = 981.0f;

    inline static float coefficientOfRestitution = 0.5f;
    inline static float particleSize = 10.0f;
    inline static float particlesCount = 100.0f;
    inline static float particlesSpacing = 25.0f;

    inline static float gridSize = 100.0f;
    inline static bool grid = false;

    inline static double deltaTime = 0.0;
    inline static double pi = 3.14159265358979323846;

    inline static float sw;
    inline static float sh;

    static void setGravity(float value) { gravity = value; }
    static void setParticleSize(float value) { particleSize = value; }
    static void setParticlesCount(float value) { particlesCount = value; }
    static void setParticlesSpacing(float value) { particlesSpacing = value; }
    static void setGrid(bool value) { grid = value; }
    static void setDeltaTime(double value) { deltaTime = value; }
};