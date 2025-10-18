#include "ProjectiveGeometry.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// Matrix4 implementation
Matrix4::Matrix4() {
    // Identity matrix
    memset(m, 0, sizeof(m));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

Matrix4::Matrix4(float m00, float m01, float m02, float m03,
                 float m10, float m11, float m12, float m13,
                 float m20, float m21, float m22, float m23,
                 float m30, float m31, float m32, float m33) {
    m[0] = m00; m[1] = m01; m[2] = m02; m[3] = m03;
    m[4] = m10; m[5] = m11; m[6] = m12; m[7] = m13;
    m[8] = m20; m[9] = m21; m[10] = m22; m[11] = m23;
    m[12] = m30; m[13] = m31; m[14] = m32; m[15] = m33;
}

Matrix4 Matrix4::perspective(float fov, float aspect, float near, float far) {
    float f = 1.0f / tanf(fov * 0.5f * M_PI / 180.0f);
    float range = 1.0f / (near - far);

    return Matrix4(
        f / aspect, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, (far + near) * range, 2.0f * far * near * range,
        0.0f, 0.0f, -1.0f, 0.0f
    );
}

Matrix4 Matrix4::orthographic(float left, float right, float bottom, float top, float near, float far) {
    float width = right - left;
    float height = top - bottom;
    float depth = far - near;

    return Matrix4(
        2.0f / width, 0.0f, 0.0f, -(right + left) / width,
        0.0f, 2.0f / height, 0.0f, -(top + bottom) / height,
        0.0f, 0.0f, -2.0f / depth, -(far + near) / depth,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Matrix4 Matrix4::lookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
    Vector3 forward = (target - eye).normalized();
    Vector3 right = forward.cross(up).normalized();
    Vector3 newUp = right.cross(forward);

    return Matrix4(
        right.x, right.y, right.z, -right.dot(eye),
        newUp.x, newUp.y, newUp.z, -newUp.dot(eye),
        -forward.x, -forward.y, -forward.z, forward.dot(eye),
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Matrix4 Matrix4::translate(const Vector3& translation) {
    return Matrix4(
        1.0f, 0.0f, 0.0f, translation.x,
        0.0f, 1.0f, 0.0f, translation.y,
        0.0f, 0.0f, 1.0f, translation.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Matrix4 Matrix4::rotate(const Vector3& axis, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    float t = 1.0f - c;
    Vector3 axisNorm = axis.normalized();

    float x = axisNorm.x;
    float y = axisNorm.y;
    float z = axisNorm.z;

    return Matrix4(
        t * x * x + c, t * x * y - s * z, t * x * z + s * y, 0.0f,
        t * x * y + s * z, t * y * y + c, t * y * z - s * x, 0.0f,
        t * x * z - s * y, t * y * z + s * x, t * z * z + c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Matrix4 Matrix4::scale(const Vector3& scale) {
    return Matrix4(
        scale.x, 0.0f, 0.0f, 0.0f,
        0.0f, scale.y, 0.0f, 0.0f,
        0.0f, 0.0f, scale.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Matrix4 Matrix4::operator*(const Matrix4& other) const {
    Matrix4 result;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result.m[i * 4 + j] += m[i * 4 + k] * other.m[k * 4 + j];
            }
        }
    }

    return result;
}

Matrix4 Matrix4::transpose() const {
    return Matrix4(
        m[0], m[4], m[8], m[12],
        m[1], m[5], m[9], m[13],
        m[2], m[6], m[10], m[14],
        m[3], m[7], m[11], m[15]
    );
}

Matrix4 Matrix4::inverse() const {
    float det = determinant();
    if (fabsf(det) < 1e-6f) {
        return Matrix4(); // Return identity for singular matrix
    }

    float invDet = 1.0f / det;

    Matrix4 result;
    result.m[0] = (m[5] * m[10] * m[15] + m[6] * m[11] * m[13] + m[7] * m[9] * m[14] -
                   m[5] * m[11] * m[14] - m[6] * m[9] * m[15] - m[7] * m[10] * m[13]) * invDet;
    result.m[1] = (m[1] * m[11] * m[14] + m[2] * m[9] * m[15] + m[3] * m[10] * m[13] -
                   m[1] * m[10] * m[15] - m[2] * m[11] * m[13] - m[3] * m[9] * m[14]) * invDet;
    result.m[2] = (m[1] * m[6] * m[15] + m[2] * m[7] * m[13] + m[3] * m[5] * m[14] -
                   m[1] * m[7] * m[14] - m[2] * m[5] * m[15] - m[3] * m[6] * m[13]) * invDet;
    result.m[3] = (m[1] * m[7] * m[10] + m[2] * m[5] * m[11] + m[3] * m[6] * m[9] -
                   m[1] * m[6] * m[11] - m[2] * m[7] * m[9] - m[3] * m[5] * m[10]) * invDet;

    result.m[4] = (m[4] * m[11] * m[14] + m[6] * m[8] * m[15] + m[7] * m[10] * m[12] -
                   m[4] * m[10] * m[15] - m[6] * m[11] * m[12] - m[7] * m[8] * m[14]) * invDet;
    result.m[5] = (m[0] * m[10] * m[15] + m[2] * m[11] * m[12] + m[3] * m[8] * m[14] -
                   m[0] * m[11] * m[14] - m[2] * m[8] * m[15] - m[3] * m[10] * m[12]) * invDet;
    result.m[6] = (m[0] * m[7] * m[14] + m[2] * m[4] * m[15] + m[3] * m[6] * m[12] -
                   m[0] * m[6] * m[15] - m[2] * m[7] * m[12] - m[3] * m[4] * m[14]) * invDet;
    result.m[7] = (m[0] * m[6] * m[11] + m[2] * m[7] * m[8] + m[3] * m[4] * m[10] -
                   m[0] * m[7] * m[10] - m[2] * m[4] * m[11] - m[3] * m[6] * m[8]) * invDet;

    result.m[8] = (m[4] * m[9] * m[15] + m[5] * m[11] * m[12] + m[7] * m[8] * m[13] -
                   m[4] * m[11] * m[13] - m[5] * m[8] * m[15] - m[7] * m[9] * m[12]) * invDet;
    result.m[9] = (m[0] * m[11] * m[13] + m[1] * m[8] * m[15] + m[3] * m[9] * m[12] -
                   m[0] * m[9] * m[15] - m[1] * m[11] * m[12] - m[3] * m[8] * m[13]) * invDet;
    result.m[10] = (m[0] * m[5] * m[15] + m[1] * m[7] * m[12] + m[3] * m[4] * m[13] -
                    m[0] * m[7] * m[13] - m[1] * m[4] * m[15] - m[3] * m[5] * m[12]) * invDet;
    result.m[11] = (m[0] * m[7] * m[9] + m[1] * m[4] * m[11] + m[3] * m[5] * m[8] -
                    m[0] * m[5] * m[11] - m[1] * m[7] * m[8] - m[3] * m[4] * m[9]) * invDet;

    result.m[12] = (m[4] * m[10] * m[13] + m[5] * m[8] * m[14] + m[6] * m[9] * m[12] -
                    m[4] * m[9] * m[14] - m[5] * m[10] * m[12] - m[6] * m[8] * m[13]) * invDet;
    result.m[13] = (m[0] * m[9] * m[14] + m[1] * m[10] * m[12] + m[2] * m[8] * m[13] -
                    m[0] * m[10] * m[13] - m[1] * m[8] * m[14] - m[2] * m[9] * m[12]) * invDet;
    result.m[14] = (m[0] * m[6] * m[13] + m[1] * m[4] * m[14] + m[2] * m[5] * m[12] -
                    m[0] * m[5] * m[14] - m[1] * m[6] * m[12] - m[2] * m[4] * m[13]) * invDet;
    result.m[15] = (m[0] * m[5] * m[10] + m[1] * m[6] * m[8] + m[2] * m[4] * m[9] -
                    m[0] * m[6] * m[9] - m[1] * m[4] * m[10] - m[2] * m[5] * m[8]) * invDet;

    return result;
}

float Matrix4::determinant() const {
    return m[3] * m[6] * m[9] * m[12] - m[2] * m[7] * m[9] * m[12] - m[3] * m[5] * m[10] * m[12] + m[1] * m[7] * m[10] * m[12] +
           m[2] * m[5] * m[11] * m[12] - m[1] * m[6] * m[11] * m[12] - m[3] * m[6] * m[8] * m[13] + m[2] * m[7] * m[8] * m[13] +
           m[3] * m[4] * m[10] * m[13] - m[0] * m[7] * m[10] * m[13] - m[2] * m[4] * m[11] * m[13] + m[0] * m[6] * m[11] * m[13] +
           m[3] * m[5] * m[8] * m[14] - m[1] * m[7] * m[8] * m[14] - m[3] * m[4] * m[9] * m[14] + m[0] * m[7] * m[9] * m[14] +
           m[1] * m[4] * m[11] * m[14] - m[0] * m[5] * m[11] * m[14] - m[2] * m[5] * m[8] * m[15] + m[1] * m[6] * m[8] * m[15] +
           m[2] * m[4] * m[9] * m[15] - m[0] * m[6] * m[9] * m[15] - m[1] * m[4] * m[10] * m[15] + m[0] * m[5] * m[10] * m[15];
}

Vector3 Matrix4::transformPoint(const Vector3& point) const {
    float w = m[12] * point.x + m[13] * point.y + m[14] * point.z + m[15];
    if (fabsf(w) < 1e-6f) return point;

    return Vector3(
        (m[0] * point.x + m[1] * point.y + m[2] * point.z + m[3]) / w,
        (m[4] * point.x + m[5] * point.y + m[6] * point.z + m[7]) / w,
        (m[8] * point.x + m[9] * point.y + m[10] * point.z + m[11]) / w
    );
}

Vector3 Matrix4::transformVector(const Vector3& vector) const {
    return Vector3(
        m[0] * vector.x + m[1] * vector.y + m[2] * vector.z,
        m[4] * vector.x + m[5] * vector.y + m[6] * vector.z,
        m[8] * vector.x + m[9] * vector.y + m[10] * vector.z
    );
}

Vector3 Matrix4::transformDirection(const Vector3& direction) const {
    return transformVector(direction).normalized();
}

// Plane implementation
Plane::Plane(const Vector3& normal, float distance)
    : normal(normal), distance(distance) {}

float Plane::distanceToPoint(const Vector3& point) const {
    return normal.dot(point) + distance;
}

float Plane::classifyPoint(const Vector3& point) const {
    return distanceToPoint(point);
}

// Frustum implementation
Frustum::Frustum(const std::array<Plane, 6>& planes) : planes(planes) {}

Frustum Frustum::extractFromMatrices(const Matrix4& projectionMatrix, const Matrix4& viewMatrix) {
    Matrix4 viewProj = projectionMatrix * viewMatrix;
    std::array<Plane, 6> planes;

    // Left plane
    planes[0] = Plane(
        Vector3(
            viewProj.m[3] + viewProj.m[0],
            viewProj.m[7] + viewProj.m[4],
            viewProj.m[11] + viewProj.m[8]
        ),
        viewProj.m[15] + viewProj.m[12]
    );

    // Right plane
    planes[1] = Plane(
        Vector3(
            viewProj.m[3] - viewProj.m[0],
            viewProj.m[7] - viewProj.m[4],
            viewProj.m[11] - viewProj.m[8]
        ),
        viewProj.m[15] - viewProj.m[12]
    );

    // Bottom plane
    planes[2] = Plane(
        Vector3(
            viewProj.m[3] + viewProj.m[1],
            viewProj.m[7] + viewProj.m[5],
            viewProj.m[11] + viewProj.m[9]
        ),
        viewProj.m[15] + viewProj.m[13]
    );

    // Top plane
    planes[3] = Plane(
        Vector3(
            viewProj.m[3] - viewProj.m[1],
            viewProj.m[7] - viewProj.m[5],
            viewProj.m[11] - viewProj.m[9]
        ),
        viewProj.m[15] - viewProj.m[13]
    );

    // Near plane
    planes[4] = Plane(
        Vector3(
            viewProj.m[3] + viewProj.m[2],
            viewProj.m[7] + viewProj.m[6],
            viewProj.m[11] + viewProj.m[10]
        ),
        viewProj.m[15] + viewProj.m[14]
    );

    // Far plane
    planes[5] = Plane(
        Vector3(
            viewProj.m[3] - viewProj.m[2],
            viewProj.m[7] - viewProj.m[6],
            viewProj.m[11] - viewProj.m[10]
        ),
        viewProj.m[15] - viewProj.m[14]
    );

    // Normalize planes
    for (auto& plane : planes) {
        float length = plane.normal.length();
        if (length > 0.0f) {
            plane.normal /= length;
            plane.distance /= length;
        }
    }

    return Frustum(planes);
}

bool Frustum::containsPoint(const Vector3& point) const {
    return std::all_of(planes.begin(), planes.end(),
        [&point](const Plane& plane) {
            return plane.classifyPoint(point) >= 0.0f;
        });
}

bool Frustum::containsSphere(const Vector3& center, float radius) const {
    return std::all_of(planes.begin(), planes.end(),
        [&center, radius](const Plane& plane) {
            return plane.distanceToPoint(center) >= -radius;
        });
}

bool Frustum::containsAABB(const Vector3& min, const Vector3& max) const {
    std::array<Vector3, 8> corners = {{
        Vector3(min.x, min.y, min.z),
        Vector3(max.x, min.y, min.z),
        Vector3(min.x, max.y, min.z),
        Vector3(max.x, max.y, min.z),
        Vector3(min.x, min.y, max.z),
        Vector3(max.x, min.y, max.z),
        Vector3(min.x, max.y, max.z),
        Vector3(max.x, max.y, max.z)
    }};

    return std::all_of(corners.begin(), corners.end(),
        [this](const Vector3& corner) {
            return containsPoint(corner);
        });
}

bool Frustum::intersectsSphere(const Vector3& center, float radius) const {
    return std::any_of(planes.begin(), planes.end(),
        [&center, radius](const Plane& plane) {
            return plane.distanceToPoint(center) < -radius;
        });
}

// Camera implementation
Camera::Camera()
    : position(0.0f, 0.0f, 0.0f)
    , rotation(0.0f, 0.0f, 0.0f, 1.0f)
    , fov(60.0f)
    , aspectRatio(16.0f / 9.0f)
    , nearPlane(0.1f)
    , farPlane(1000.0f) {}

Matrix4 Camera::getViewMatrix() const {
    Vector3 forward = rotation * Vector3(0.0f, 0.0f, -1.0f);
    Vector3 up = rotation * Vector3(0.0f, 1.0f, 0.0f);
    return Matrix4::lookAt(position, position + forward, up);
}

Matrix4 Camera::getProjectionMatrix() const {
    return Matrix4::perspective(fov, aspectRatio, nearPlane, farPlane);
}

Matrix4 Camera::getViewProjectionMatrix() const {
    return getProjectionMatrix() * getViewMatrix();
}

Frustum Camera::getFrustum() const {
    return Frustum::extractFromMatrices(getProjectionMatrix(), getViewMatrix());
}

Vector3 Camera::screenToWorld(const Vector2& screenPoint, const Vector2& screenSize) const {
    // Convert screen coordinates to normalized device coordinates
    float ndcX = (2.0f * screenPoint.x / screenSize.x) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenPoint.y / screenSize.y);

    // Create ray in NDC
    Vector3 rayNdc(ndcX, ndcY, 1.0f);

    // Transform to world space
    Matrix4 invViewProj = getViewProjectionMatrix().inverse();
    Vector3 rayWorld = invViewProj.transformPoint(rayNdc);

    return rayWorld.normalized();
}

Vector2 Camera::worldToScreen(const Vector3& worldPoint, const Vector2& screenSize) const {
    Vector3 clipSpace = getViewProjectionMatrix().transformPoint(worldPoint);

    // Convert to screen coordinates
    float screenX = ((clipSpace.x + 1.0f) * 0.5f) * screenSize.x;
    float screenY = ((1.0f - clipSpace.y) * 0.5f) * screenSize.y;

    return Vector2(screenX, screenY);
}

void Camera::lookAt(const Vector3& target, const Vector3& up) {
    Vector3 forward = (target - position).normalized();
    Vector3 right = forward.cross(up).normalized();
    Vector3 newUp = right.cross(forward);

    // Create rotation matrix and convert to quaternion
    Matrix4 rotMatrix(
        right.x, newUp.x, -forward.x, 0.0f,
        right.y, newUp.y, -forward.y, 0.0f,
        right.z, newUp.z, -forward.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    rotation = quaternionFromMatrix(rotMatrix);
}

Quaternion Camera::quaternionFromMatrix(const Matrix4& matrix) {
    float trace = matrix.m[0] + matrix.m[5] + matrix.m[10];

    if (trace > 0.0f) {
        float s = sqrtf(trace + 1.0f) * 2.0f;
        return Quaternion(
            (matrix.m[9] - matrix.m[6]) / s,
            (matrix.m[2] - matrix.m[8]) / s,
            (matrix.m[4] - matrix.m[1]) / s,
            s * 0.25f
        );
    } else if (matrix.m[0] > matrix.m[5] && matrix.m[0] > matrix.m[10]) {
        float s = sqrtf(1.0f + matrix.m[0] - matrix.m[5] - matrix.m[10]) * 2.0f;
        return Quaternion(
            s * 0.25f,
            (matrix.m[4] + matrix.m[1]) / s,
            (matrix.m[2] + matrix.m[8]) / s,
            (matrix.m[9] - matrix.m[6]) / s
        );
    } else if (matrix.m[5] > matrix.m[10]) {
        float s = sqrtf(1.0f + matrix.m[5] - matrix.m[0] - matrix.m[10]) * 2.0f;
        return Quaternion(
            (matrix.m[4] + matrix.m[1]) / s,
            s * 0.25f,
            (matrix.m[9] + matrix.m[6]) / s,
            (matrix.m[2] - matrix.m[8]) / s
        );
    } else {
        float s = sqrtf(1.0f + matrix.m[10] - matrix.m[0] - matrix.m[5]) * 2.0f;
        return Quaternion(
            (matrix.m[2] + matrix.m[8]) / s,
            (matrix.m[9] + matrix.m[6]) / s,
            s * 0.25f,
            (matrix.m[4] - matrix.m[1]) / s
        );
    }
}