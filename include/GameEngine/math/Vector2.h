#ifndef FOUNDRY_GAMEENGINE_VECTOR2_H
#define FOUNDRY_GAMEENGINE_VECTOR2_H

#include <cmath>

namespace FoundryEngine {

class Vector2 {
public:
    float x, y;

    Vector2(float x_ = 0.0f, float y_ = 0.0f) : x(x_), y(y_) {}

    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2 operator-() const {
        return Vector2(-x, -y);
    }

    Vector2& operator+=(const Vector2& other) {
        x += other.x; y += other.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& other) {
        x -= other.x; y -= other.y;
        return *this;
    }

    Vector2& operator*=(float scalar) {
        x *= scalar; y *= scalar;
        return *this;
    }

    float magnitude() const {
        return sqrt(x*x + y*y);
    }

    float magnitudeSq() const {
        return x*x + y*y;
    }

    Vector2 normalized() const {
        float mag = magnitude();
        if (mag > 0) {
            return *this * (1.0f / mag);
        }
        return Vector2(0, 0);
    }

    float dot(const Vector2& other) const {
        return x*other.x + y*other.y;
    }

    Vector2 lerp(const Vector2& other, float t) const {
        return *this + (other - *this) * t;
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_VECTOR2_H
