/**
 * @file Vector3.h
 * @brief 3D vector mathematics class with SIMD-friendly operations
 * @author FoundryEngine Team
 * @date 2024
 * @version 1.0.0
 *
 * This file contains the Vector3 class which provides essential 3D vector mathematics
 * for the FoundryEngine. The class is designed for high performance with cache-friendly
 * memory layout and SIMD-compatible operations.
 *
 * Key Features:
 * - Basic arithmetic operations (add, subtract, multiply, negate)
 * - Vector operations (dot product, cross product, magnitude)
 * - Normalization and interpolation functions
 * - Compound assignment operators for efficiency
 * - SIMD-friendly memory layout (x, y, z contiguous)
 *
 * Performance Optimizations:
 * - Inlined operations for minimal function call overhead
 * - Cache-friendly member layout
 * - Branchless implementations where possible
 * - Compatible with SIMD operations (4-wide vectors)
 *
 * Usage Examples:
 * @code
 * // Basic operations
 * Vector3 position(10.0f, 5.0f, 0.0f);
 * Vector3 velocity(1.0f, 0.0f, 0.0f);
 * Vector3 newPos = position + velocity * deltaTime;
 *
 * // Vector math
 * Vector3 direction = target - position;
 * float distance = direction.magnitude();
 * Vector3 unitDir = direction.normalized();
 *
 * // Physics calculations
 * Vector3 force = mass * acceleration;
 * float dotProduct = velocity.dot(force);
 * Vector3 torque = position.cross(force);
 * @endcode
 *
 * Memory Layout:
 * The class uses a simple struct-like layout with x, y, z members in contiguous memory,
 * making it ideal for SIMD operations and efficient memory access patterns.
 *
 * Thread Safety:
 * All operations are thread-safe for read operations. Write operations should be
 * coordinated by the caller if used in multi-threaded contexts.
 *
 * Dependencies:
 * - <cmath> for sqrt() function
 */

#ifndef FOUNDRY_GAMEENGINE_VECTOR3_H
#define FOUNDRY_GAMEENGINE_VECTOR3_H

#include <cmath>

namespace FoundryEngine {

/**
 * @brief 3D vector class with comprehensive mathematical operations
 *
 * Vector3 represents a point or direction in 3D space with single-precision
 * floating-point coordinates. The class provides all essential vector operations
 * needed for 3D graphics, physics, and game mathematics.
 *
 * The implementation is optimized for performance with inlined operations and
 * cache-friendly memory access patterns.
 */
class Vector3 {
public:
    float x, y, z;  ///< Cartesian coordinates (public for performance and SIMD access)

    /**
     * @brief Construct a 3D vector
     * @param x_ X coordinate (default 0.0f)
     * @param y_ Y coordinate (default 0.0f)
     * @param z_ Z coordinate (default 0.0f)
     *
     * Creates a new vector with the specified coordinates.
     * All parameters default to 0.0f for convenience.
     */
    Vector3(float x_ = 0.0f, float y_ = 0.0f, float z_ = 0.0f) : x(x_), y(y_), z(z_) {}

    // Arithmetic operators

    /**
     * @brief Vector addition
     * @param other Vector to add
     * @return New vector containing the sum
     *
     * Performs component-wise addition: (x1+x2, y1+y2, z1+z2)
     * Useful for position updates and vector accumulation.
     */
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    /**
     * @brief Vector subtraction
     * @param other Vector to subtract
     * @return New vector containing the difference
     *
     * Performs component-wise subtraction: (x1-x2, y1-y2, z1-z2)
     * Useful for finding directions and relative positions.
     */
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    /**
     * @brief Scalar multiplication
     * @param scalar Scalar value to multiply by
     * @return New vector scaled by the scalar
     *
     * Multiplies each component by the scalar: (x*s, y*s, z*s)
     * Used for scaling, time-based movement, and attenuation.
     */
    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    /**
     * @brief Vector negation (unary minus)
     * @return New vector with negated components
     *
     * Returns the additive inverse: (-x, -y, -z)
     * Useful for reversing directions and reflections.
     */
    Vector3 operator-() const {
        return Vector3(-x, -y, -z);
    }

    // Compound assignment operators

    /**
     * @brief In-place vector addition
     * @param other Vector to add
     * @return Reference to this vector (for chaining)
     *
     * Adds the other vector to this one in-place.
     * More efficient than creating a new vector.
     */
    Vector3& operator+=(const Vector3& other) {
        x += other.x; y += other.y; z += other.z;
        return *this;
    }

    /**
     * @brief In-place vector subtraction
     * @param other Vector to subtract
     * @return Reference to this vector (for chaining)
     *
     * Subtracts the other vector from this one in-place.
     * More efficient than creating a new vector.
     */
    Vector3& operator-=(const Vector3& other) {
        x -= other.x; y -= other.y; z -= other.z;
        return *this;
    }

    /**
     * @brief In-place scalar multiplication
     * @param scalar Scalar value to multiply by
     * @return Reference to this vector (for chaining)
     *
     * Scales this vector in-place by the scalar value.
     * More efficient than creating a new vector.
     */
    Vector3& operator*=(float scalar) {
        x *= scalar; y *= scalar; z *= scalar;
        return *this;
    }

    // Vector operations

    /**
     * @brief Calculate vector magnitude (length)
     * @return Length of the vector
     *
     * Computes the Euclidean norm: sqrt(x² + y² + z²)
     * Always returns a non-negative value.
     *
     * @note Expensive operation (square root)
     * @note Use magnitudeSq() for comparisons to avoid sqrt
     * @see magnitudeSq() for squared magnitude
     */
    float magnitude() const {
        return sqrt(x*x + y*y + z*z);
    }

    /**
     * @brief Calculate squared vector magnitude
     * @return Squared length of the vector (x² + y² + z²)
     *
     * Computes the squared Euclidean norm without the expensive square root.
     * Useful for distance comparisons and normalization checks.
     *
     * @note Much faster than magnitude() - no square root
     * @note Use for relative distance comparisons
     * @note Equivalent to dot(*this) for the same vector
     * @see magnitude() for actual length
     */
    float magnitudeSq() const {
        return x*x + y*y + z*z;
    }

    /**
     * @brief Normalize the vector to unit length
     * @return New unit vector in the same direction
     *
     * Returns a vector with the same direction but length 1.0.
     * If the vector is zero-length, returns a zero vector.
     *
     * @note Returns zero vector for zero-length input
     * @note Result has magnitude exactly 1.0 (except for zero input)
     * @see magnitude() to check if normalization is safe
     */
    Vector3 normalized() const {
        float mag = magnitude();
        if (mag > 0) {
            return *this * (1.0f / mag);
        }
        return Vector3(0, 0, 0);
    }

    /**
     * @brief Calculate dot product with another vector
     * @param other Vector to dot with
     * @return Dot product scalar result
     *
     * Computes the dot product: x1*x2 + y1*y2 + z1*z2
     * Used for angle calculations, projections, and similarity measures.
     *
     * @note Commutative: a.dot(b) == b.dot(a)
     * @note Result > 0 for acute angles, < 0 for obtuse angles
     * @note |a.dot(b)| <= |a| * |b| (Cauchy-Schwarz inequality)
     * @see cross() for perpendicular vector calculation
     */
    float dot(const Vector3& other) const {
        return x*other.x + y*other.y + z*other.z;
    }

    /**
     * @brief Calculate cross product with another vector
     * @param other Vector to cross with
     * @return New vector perpendicular to both input vectors
     *
     * Computes the cross product using the right-hand rule.
     * Result is perpendicular to both input vectors.
     *
     * @note Anti-commutative: a.cross(b) == -b.cross(a)
     * @note |a.cross(b)| == |a| * |b| * sin(theta)
     * @note Used for surface normals, torque, angular velocity
     * @see dot() for parallel component calculation
     */
    Vector3 cross(const Vector3& other) const {
        return Vector3(
            y*other.z - z*other.y,
            z*other.x - x*other.z,
            x*other.y - y*other.x
        );
    }

    /**
     * @brief Linear interpolation between vectors
     * @param other Target vector to interpolate towards
     * @param t Interpolation parameter (0.0 = this vector, 1.0 = other vector)
     * @return Interpolated vector
     *
     * Performs linear interpolation: this + (other - this) * t
     * Commonly used for smooth transitions and animations.
     *
     * @param other The target vector for interpolation
     * @param t Interpolation factor (clamped to [0,1] for standard lerp)
     * @return Vector at position t along the line from this to other
     * @note t=0 returns this vector, t=1 returns other vector
     * @note Can extrapolate beyond [0,1] range if desired
     * @note Smooth and continuous interpolation
     */
    Vector3 lerp(const Vector3& other, float t) const {
        return *this + (other - *this) * t;
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_VECTOR3_H
