#pragma once
#include <cmath>
#include <algorithm>

#include "particle.h"
#include "globals.h"

class Physics2D
{
public:
    float getFinalPosition(float acceleration, float velocity, float position, float time)
    {
        float x = 0.5 * acceleration * pow(time, 2) + velocity * time + position;

        return x;
    }

    float getTotalTimeUntilFinalPostion(float initialPosition, float finalPosition, float velocity, float acceleration)
    {
        float discriminant = velocity * velocity - 2 * acceleration * (initialPosition - finalPosition);
        if (discriminant < 0)
            return NAN; // no real solution

        float t1 = (-velocity + sqrt(discriminant)) / acceleration;
        float t2 = (-velocity - sqrt(discriminant)) / acceleration;

        if (t1 >= 0 && t2 >= 0)
            return std::min(t1, t2);
        else if (t1 >= 0)
            return t1;
        else
            return t2;
    }

    float getVectorDirectionAngle(const float &objX, const float &objY,
                                  const float &centerX = 0, const float &centerY = 0,
                                  const bool &convertRadianToAngle = false)
    {
        float x = objX - centerX;
        float y = objY - centerY;

        float angle = atan2(y, x);

        if (convertRadianToAngle)
        {
            float degrees = angle * 180.0f / PI;

            if (degrees < 0)
                degrees += 360.0f;

            return degrees;
        }
        else
            return angle;
    }

    float getVectorMagnitude(const float &objX, const float &objY,
                             const float &centerX = 0, const float &centerY = 0)
    {
        float a = objX - centerX;
        float b = objY - centerY;

        return sqrt((a * a) + (b * b));
    }

    float getXForVectorUsingAngleAndMagnitude(const float &angle, const float &magnitude, const float &additionalValue = 0)
    {
        return magnitude * cos(angle) + additionalValue;
    }

    float getYForVectorUsingAngleAndMagnitude(const float &angle, const float &magnitude, const float &additionalValue = 0)
    {
        return magnitude * sin(angle) + additionalValue;
    }

    // void updatePositionXBasedOnVXAndDeltaTime(Object &obj)
    // {
    //     obj.x += obj.vx * deltaTime;
    // }

    // void updatePositionYBasedOnVYAndDeltaTime(Object &obj)
    // {
    //     obj.y += obj.vy * deltaTime;
    // }
};