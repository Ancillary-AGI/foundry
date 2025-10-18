#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"
#include "Color.h"
#include <array>

/**
 * Projective Geometry utilities for 3D game engine
 * Essential for 3D rendering, camera systems, and spatial transformations
 */

// Forward declarations
class Vector2;
class Vector3;
class Vector4;
class Quaternion;
class Color;

// Matrix4 class for 4x4 transformation matrices
class Matrix4 {
public:
    float m[16]; // Column-major order

    Matrix4();
    Matrix4(float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33);

    // Static factory methods
    static Matrix4 perspective(float fov, float aspect, float near, float far);
    static Matrix4 orthographic(float left, float right, float bottom, float top, float near, float far);
    static Matrix4 lookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
    static Matrix4 translate(const Vector3& translation);
    static Matrix4 rotate(const Vector3& axis, float angle);
    static Matrix4 scale(const Vector3& scale);

    // Matrix operations
    Matrix4 operator*(const Matrix4& other) const;
    Matrix4 transpose() const;
    Matrix4 inverse() const;
    float determinant() const;

    // Transformations
    Vector3 transformPoint(const Vector3& point) const;
    Vector3 transformVector(const Vector3& vector) const;
    Vector3 transformDirection(const Vector3& direction) const;
};

// Plane class for frustum planes
class Plane {
public:
    Vector3 normal;
    float distance;

    Plane(const Vector3& normal, float distance);

    float distanceToPoint(const Vector3& point) const;
    float classifyPoint(const Vector3& point) const;
};

// Frustum class for view frustum culling
class Frustum {
public:
    std::array<Plane, 6> planes;

    Frustum(const std::array<Plane, 6>& planes);

    static Frustum extractFromMatrices(const Matrix4& projectionMatrix, const Matrix4& viewMatrix);

    bool containsPoint(const Vector3& point) const;
    bool containsSphere(const Vector3& center, float radius) const;
    bool containsAABB(const Vector3& min, const Vector3& max) const;
    bool intersectsSphere(const Vector3& center, float radius) const;
};

// Camera class for 3D camera management
class Camera {
public:
    Vector3 position;
    Quaternion rotation;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;

    Camera();

    Matrix4 getViewMatrix() const;
    Matrix4 getProjectionMatrix() const;
    Matrix4 getViewProjectionMatrix() const;
    Frustum getFrustum() const;

    Vector3 screenToWorld(const Vector2& screenPoint, const Vector2& screenSize) const;
    Vector2 worldToScreen(const Vector3& worldPoint, const Vector2& screenSize) const;

    void lookAt(const Vector3& target, const Vector3& up = Vector3(0.0f, 1.0f, 0.0f));

private:
    Quaternion quaternionFromMatrix(const Matrix4& matrix);
};

// C-style functions for WebAssembly/TypeScript binding
extern "C" {
    // Matrix4 functions
    Matrix4 Matrix4_perspective(float fov, float aspect, float near, float far);
    Matrix4 Matrix4_orthographic(float left, float right, float bottom, float top, float near, float far);
    Matrix4 Matrix4_lookAt(float eyeX, float eyeY, float eyeZ, float targetX, float targetY, float targetZ, float upX, float upY, float upZ);
    Matrix4 Matrix4_multiply(Matrix4 a, Matrix4 b);
    Vector3 Matrix4_transformPoint(Matrix4 matrix, float x, float y, float z);
    Vector3 Matrix4_transformVector(Matrix4 matrix, float x, float y, float z);

    // Frustum functions
    bool Frustum_containsPoint(const Frustum* frustum, float x, float y, float z);

    // Camera functions
    Vector3 Camera_screenToWorld(const Camera* camera, float screenX, float screenY, float screenWidth, float screenHeight);
    Vector2 Camera_worldToScreen(const Camera* camera, float worldX, float worldY, float worldZ, float screenWidth, float screenHeight);
}