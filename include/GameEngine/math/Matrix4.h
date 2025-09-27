#ifndef NEUTRAL_GAMEENGINE_MATRIX4_H
#define NEUTRAL_GAMEENGINE_MATRIX4_H

#include "Vector3.h"
#include <array>
#include <cmath>

namespace FoundryEngine {

class Matrix4 {
public:
    std::array<std::array<float, 4>, 4> m;

    Matrix4() : m{{{}} {}} {}

    static Matrix4 identity() {
        Matrix4 mat;
        for (int i = 0; i < 4; ++i) mat.m[i][i] = 1.0f;
        return mat;
    }

    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i][j] = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    result.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return result;
    }

    Vector3 operator*(const Vector3& v) const {
        float x = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3];
        float y = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3];
        float z = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3];
        return Vector3(x, y, z);
    }

    Matrix4& translate(const Vector3& v) {
        m[0][3] += v.x;
        m[1][3] += v.y;
        m[2][3] += v.z;
        return *this;
    }

    Matrix4& scale(const Vector3& v) {
        for (int i = 0; i < 3; ++i) {
            m[i][0] *= v.x;
            m[i][1] *= v.y;
            m[i][2] *= v.z;
        }
        return *this;
    }

    // Simplify for now, full rotations if needed
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_MATRIX4_H
