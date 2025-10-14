#ifndef FOUNDRY_GAMEENGINE_QUATERNION_H
#define FOUNDRY_GAMEENGINE_QUATERNION_H

#include "Vector3.h"
#include <cmath>

namespace FoundryEngine {

class Quaternion {
public:
    float w, x, y, z;

    Quaternion(float w_ = 1.0f, float x_ = 0.0f, float y_ = 0.0f, float z_ = 0.0f) : w(w_), x(x_), y(y_), z(z_) {}

    // From Euler angles (ZYX order)
    static Quaternion fromEuler(float pitch, float yaw, float roll) {
        float cr = cos(roll * 0.5f);
        float sr = sin(roll * 0.5f);
        float cp = cos(pitch * 0.5f);
        float sp = sin(pitch * 0.5f);
        float cy = cos(yaw * 0.5f);
        float sy = sin(yaw * 0.5f);

        return Quaternion(
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy
        );
    }

    // From axis-angle
    static Quaternion fromAxisAngle(const Vector3& axis, float angle) {
        Vector3 normalizedAxis = axis.normalized();
        float halfAngle = angle * 0.5f;
        float sinHalf = sin(halfAngle);

        return Quaternion(
            cos(halfAngle),
            normalizedAxis.x * sinHalf,
            normalizedAxis.y * sinHalf,
            normalizedAxis.z * sinHalf
        );
    }

    Quaternion operator*(const Quaternion& other) const {
        return Quaternion(
            w * other.w - x * other.x - y * other.y - z * other.z,
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w
        );
    }

    Quaternion normalized() const {
        float mag = sqrt(w*w + x*x + y*y + z*z);
        if (mag > 0) {
            return Quaternion(w/mag, x/mag, y/mag, z/mag);
        }
        return Quaternion();
    }

    Quaternion inverse() const {
        float norm = w*w + x*x + y*y + z*z;
        return Quaternion(w/norm, -x/norm, -y/norm, -z/norm);
    }

    Vector3 rotate(const Vector3& v) const {
        Quaternion qv(0, v.x, v.y, v.z);
        Quaternion result = *this * qv * inverse();
        return Vector3(result.x, result.y, result.z);
    }

    // SLERP for spherical interpolation
    static Quaternion slerp(const Quaternion& a, const Quaternion& b, float t) {
        float cosHalfTheta = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
        if (abs(cosHalfTheta) >= 1.0f) return a;

        float halfTheta = acos(cosHalfTheta);
        float sinHalfTheta = sqrt(1.0f - cosHalfTheta * cosHalfTheta);

        if (abs(sinHalfTheta) < 0.001f) {
            return Quaternion(
                (a.w + b.w) * 0.5f,
                (a.x + b.x) * 0.5f,
                (a.y + b.y) * 0.5f,
                (a.z + b.z) * 0.5f
            );
        }

        float ratioA = sin((1 - t) * halfTheta) / sinHalfTheta;
        float ratioB = sin(t * halfTheta) / sinHalfTheta;

        return Quaternion(
            a.w * ratioA + b.w * ratioB,
            a.x * ratioA + b.x * ratioB,
            a.y * ratioA + b.y * ratioB,
            a.z * ratioA + b.z * ratioB
        ).normalized();
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_QUATERNION_H
