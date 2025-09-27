#ifndef NEUTRAL_GAMEENGINE_VECTOR2_H
#define NEUTRAL_GAMEENGINE_VECTOR2_H

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

    Vector2 operator/(float scalar) const {
        return Vector2(x / scalar, y / scalar);
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
            return *this / mag;
        }
        return Vector2(0, 0);
    }

    float dot(const Vector2& other) const {
        return x*other.x + y*other.y;
    }

    float cross(const Vector2& other) const {
        return x * other.y - y * other.x;
    }

    Vector2 perpendicular() const {
        return Vector2(y, -x); // CW perpendicular
    }

    Vector2 reflect(const Vector2& normal) const {
        return *this - 2 * dot(normal) * normal;
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_VECTOR2_H
