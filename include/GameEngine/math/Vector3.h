#ifndef FOUNDRY_GAMEENGINE_VECTOR3_H
#define FOUNDRY_GAMEENGINE_VECTOR3_H

#include <cmath>

namespace FoundryEngine {

class Vector3 {
public:
    float x, y, z;

    Vector3(float x_ = 0.0f, float y_ = 0.0f, float z_ = 0.0f) : x(x_), y(y_), z(z_) {}

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3 operator-() const {
        return Vector3(-x, -y, -z);
    }

    Vector3& operator+=(const Vector3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }

    Vector3& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }

    float magnitude() const {
        return sqrt(x*x + y*y + z*z);
    }

    float magnitudeSq() const {
        return x*x + y*y + z*z;
    }

    Vector3 normalized() const {
        float mag = magnitude();
        if (mag > 0) {
            return *this * (1.0f / mag);
        }
        return Vector3(0, 0, 0);
    }

    float dot(const Vector3& other) const {
        return x*other.x + y*other.y + z*other.z;
    }

    Vector3 cross(const Vector3& other) const {
        return Vector3(
            y*other.z - z*other.y,
            z*other.x - x*other.z,
            x*other.y - y*other.x
        );
    }

    Vector3 lerp(const Vector3& other, float t) const {
        return *this + (other - *this) * t;
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_VECTOR3_H
