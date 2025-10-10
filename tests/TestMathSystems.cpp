#include "gtest/gtest.h"
#include "GameEngine/math/Vector2.h"
#include "GameEngine/math/Vector3.h"
#include "GameEngine/math/Vector4.h"
#include "GameEngine/math/Matrix4.h"
#include "GameEngine/math/Quaternion.h"
#include "GameEngine/math/Polynomial.h"
#include "GameEngine/math/NumericalMethods.h"
#include <cmath>
#include <thread>
#include <chrono>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Math Systems tests
 */
class MathSystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test vectors
        vec2 = Vector2(3.0f, 4.0f);
        vec3 = Vector3(1.0f, 2.0f, 3.0f);
        vec4 = Vector4(1.0f, 2.0f, 3.0f, 4.0f);
        quat = Quaternion(0.0f, 1.0f, 0.0f, 1.0f);
        matrix = Matrix4();
        matrix.m[0] = 1.0f; matrix.m[5] = 1.0f; matrix.m[10] = 1.0f; matrix.m[15] = 1.0f;
    }

    Vector2 vec2;
    Vector3 vec3;
    Vector4 vec4;
    Quaternion quat;
    Matrix4 matrix;
};

/**
 * @brief Test Vector2 functionality
 */
TEST_F(MathSystemsTest, Vector2Operations) {
    Vector2 a(3.0f, 4.0f);
    Vector2 b(1.0f, 2.0f);

    // Test basic arithmetic
    Vector2 sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 4.0f);
    EXPECT_FLOAT_EQ(sum.y, 6.0f);

    Vector2 diff = a - b;
    EXPECT_FLOAT_EQ(diff.x, 2.0f);
    EXPECT_FLOAT_EQ(diff.y, 2.0f);

    Vector2 product = a * 2.0f;
    EXPECT_FLOAT_EQ(product.x, 6.0f);
    EXPECT_FLOAT_EQ(product.y, 8.0f);

    Vector2 quotient = a / 2.0f;
    EXPECT_FLOAT_EQ(quotient.x, 1.5f);
    EXPECT_FLOAT_EQ(quotient.y, 2.0f);

    // Test dot product
    float dot = Vector2::dot(a, b);
    EXPECT_FLOAT_EQ(dot, 3.0f * 1.0f + 4.0f * 2.0f);

    // Test magnitude
    float magnitude = a.magnitude();
    EXPECT_FLOAT_EQ(magnitude, 5.0f); // 3-4-5 triangle

    // Test normalization
    Vector2 normalized = a.normalized();
    EXPECT_FLOAT_EQ(normalized.magnitude(), 1.0f);

    // Test distance
    float distance = Vector2::distance(a, b);
    EXPECT_FLOAT_EQ(distance, 2.828427f); // sqrt((2)^2 + (2)^2)

    // Test angle
    float angle = Vector2::angle(a, b);
    EXPECT_GT(angle, 0.0f);
    EXPECT_LT(angle, 1.57f); // Less than 90 degrees

    // Test lerp
    Vector2 lerped = Vector2::lerp(a, b, 0.5f);
    EXPECT_FLOAT_EQ(lerped.x, 2.0f);
    EXPECT_FLOAT_EQ(lerped.y, 3.0f);

    // Test clamp
    Vector2 clamped = Vector2::clamp(a, Vector2(0.0f, 0.0f), Vector2(3.5f, 4.5f));
    EXPECT_FLOAT_EQ(clamped.x, 3.0f);
    EXPECT_FLOAT_EQ(clamped.y, 4.0f);
}

/**
 * @brief Test Vector3 functionality
 */
TEST_F(MathSystemsTest, Vector3Operations) {
    Vector3 a(1.0f, 2.0f, 3.0f);
    Vector3 b(4.0f, 5.0f, 6.0f);

    // Test basic arithmetic
    Vector3 sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);

    Vector3 cross = Vector3::cross(a, b);
    EXPECT_FLOAT_EQ(cross.x, 2.0f * 6.0f - 3.0f * 5.0f);
    EXPECT_FLOAT_EQ(cross.y, 3.0f * 4.0f - 1.0f * 6.0f);
    EXPECT_FLOAT_EQ(cross.z, 1.0f * 5.0f - 2.0f * 4.0f);

    // Test dot product
    float dot = Vector3::dot(a, b);
    EXPECT_FLOAT_EQ(dot, 1.0f * 4.0f + 2.0f * 5.0f + 3.0f * 6.0f);

    // Test magnitude
    float magnitude = a.magnitude();
    EXPECT_FLOAT_EQ(magnitude, sqrtf(1.0f + 4.0f + 9.0f));

    // Test normalization
    Vector3 normalized = a.normalized();
    EXPECT_FLOAT_EQ(normalized.magnitude(), 1.0f);

    // Test distance
    float distance = Vector3::distance(a, b);
    EXPECT_FLOAT_EQ(distance, sqrtf(9.0f + 9.0f + 9.0f));

    // Test projection
    Vector3 projection = Vector3::project(a, b);
    float projLength = Vector3::dot(a, b) / b.magnitude();
    EXPECT_FLOAT_EQ(projection.magnitude(), projLength);

    // Test reflection
    Vector3 normal(0.0f, 1.0f, 0.0f);
    Vector3 reflected = Vector3::reflect(a, normal);
    EXPECT_FLOAT_EQ(reflected.y, -a.y);

    // Test lerp
    Vector3 lerped = Vector3::lerp(a, b, 0.3f);
    EXPECT_FLOAT_EQ(lerped.x, 1.0f + 0.3f * (4.0f - 1.0f));
    EXPECT_FLOAT_EQ(lerped.y, 2.0f + 0.3f * (5.0f - 2.0f));
    EXPECT_FLOAT_EQ(lerped.z, 3.0f + 0.3f * (6.0f - 3.0f));
}

/**
 * @brief Test Vector4 functionality
 */
TEST_F(MathSystemsTest, Vector4Operations) {
    Vector4 a(1.0f, 2.0f, 3.0f, 4.0f);
    Vector4 b(2.0f, 3.0f, 4.0f, 5.0f);

    // Test basic arithmetic
    Vector4 sum = a + b;
    EXPECT_FLOAT_EQ(sum.x, 3.0f);
    EXPECT_FLOAT_EQ(sum.y, 5.0f);
    EXPECT_FLOAT_EQ(sum.z, 7.0f);
    EXPECT_FLOAT_EQ(sum.w, 9.0f);

    // Test homogeneous operations
    Vector3 homogeneous = a.xyz();
    EXPECT_EQ(homogeneous, Vector3(1.0f, 2.0f, 3.0f));

    Vector4 homogeneous4 = Vector4::fromVector3(homogeneous, 1.0f);
    EXPECT_EQ(homogeneous4, a);

    // Test dot product
    float dot = Vector4::dot(a, b);
    EXPECT_FLOAT_EQ(dot, 1.0f * 2.0f + 2.0f * 3.0f + 3.0f * 4.0f + 4.0f * 5.0f);

    // Test magnitude
    float magnitude = a.magnitude();
    EXPECT_FLOAT_EQ(magnitude, sqrtf(1.0f + 4.0f + 9.0f + 16.0f));

    // Test normalization
    Vector4 normalized = a.normalized();
    EXPECT_FLOAT_EQ(normalized.magnitude(), 1.0f);
}

/**
 * @brief Test Matrix4 functionality
 */
TEST_F(MathSystemsTest, Matrix4Operations) {
    Matrix4 identity;
    identity.m[0] = 1.0f; identity.m[5] = 1.0f;
    identity.m[10] = 1.0f; identity.m[15] = 1.0f;

    Matrix4 translation;
    translation.m[0] = 1.0f; translation.m[5] = 1.0f; translation.m[10] = 1.0f; translation.m[15] = 1.0f;
    translation.m[12] = 5.0f; translation.m[13] = 10.0f; translation.m[14] = 15.0f;

    // Test matrix multiplication
    Matrix4 result = identity * translation;
    EXPECT_FLOAT_EQ(result.m[12], 5.0f);
    EXPECT_FLOAT_EQ(result.m[13], 10.0f);
    EXPECT_FLOAT_EQ(result.m[14], 15.0f);

    // Test vector transformation
    Vector4 testVec(1.0f, 2.0f, 3.0f, 1.0f);
    Vector4 transformed = translation * testVec;
    EXPECT_FLOAT_EQ(transformed.x, 6.0f);  // 1 + 5
    EXPECT_FLOAT_EQ(transformed.y, 12.0f); // 2 + 10
    EXPECT_FLOAT_EQ(transformed.z, 18.0f); // 3 + 15
    EXPECT_FLOAT_EQ(transformed.w, 1.0f);

    // Test matrix inversion
    Matrix4 inverse = identity.inverse();
    Matrix4 identityCheck = identity * inverse;
    EXPECT_FLOAT_EQ(identityCheck.m[0], 1.0f);
    EXPECT_FLOAT_EQ(identityCheck.m[5], 1.0f);
    EXPECT_FLOAT_EQ(identityCheck.m[10], 1.0f);
    EXPECT_FLOAT_EQ(identityCheck.m[15], 1.0f);

    // Test transpose
    Matrix4 transpose = identity.transpose();
    EXPECT_FLOAT_EQ(transpose.m[0], 1.0f);
    EXPECT_FLOAT_EQ(transpose.m[5], 1.0f);
    EXPECT_FLOAT_EQ(transpose.m[10], 1.0f);
    EXPECT_FLOAT_EQ(transpose.m[15], 1.0f);

    // Test determinant
    float det = identity.determinant();
    EXPECT_FLOAT_EQ(det, 1.0f);

    // Test rotation matrix
    Matrix4 rotation = Matrix4::rotateX(1.57f); // 90 degrees
    Vector4 rotateVec(0.0f, 1.0f, 0.0f, 1.0f);
    Vector4 rotated = rotation * rotateVec;
    EXPECT_NEAR(rotated.x, 0.0f, 0.001f);
    EXPECT_NEAR(rotated.y, 0.0f, 0.001f);
    EXPECT_NEAR(rotated.z, 1.0f, 0.001f);
}

/**
 * @brief Test Quaternion functionality
 */
TEST_F(MathSystemsTest, QuaternionOperations) {
    Quaternion q1(0.0f, 1.0f, 0.0f, 1.0f); // 90 degree rotation around Y
    Quaternion q2(1.0f, 0.0f, 0.0f, 1.0f); // 90 degree rotation around X

    // Test quaternion multiplication
    Quaternion product = q1 * q2;
    EXPECT_NE(product.x, 0.0f);
    EXPECT_NE(product.y, 0.0f);
    EXPECT_NE(product.z, 0.0f);
    EXPECT_NE(product.w, 0.0f);

    // Test quaternion conjugation
    Quaternion conjugate = q1.conjugate();
    EXPECT_FLOAT_EQ(conjugate.x, -q1.x);
    EXPECT_FLOAT_EQ(conjugate.y, -q1.y);
    EXPECT_FLOAT_EQ(conjugate.z, -q1.z);
    EXPECT_FLOAT_EQ(conjugate.w, q1.w);

    // Test quaternion normalization
    Quaternion normalized = q1.normalized();
    EXPECT_FLOAT_EQ(normalized.magnitude(), 1.0f);

    // Test quaternion to matrix conversion
    Matrix4 rotationMatrix = q1.toMatrix4();
    EXPECT_FLOAT_EQ(rotationMatrix.m[0], 0.0f);  // cos(90) = 0
    EXPECT_FLOAT_EQ(rotationMatrix.m[2], -1.0f); // -sin(90) = -1
    EXPECT_FLOAT_EQ(rotationMatrix.m[8], 1.0f);  // sin(90) = 1
    EXPECT_FLOAT_EQ(rotationMatrix.m[10], 0.0f); // cos(90) = 0

    // Test spherical linear interpolation
    Quaternion qStart(0.0f, 0.0f, 0.0f, 1.0f); // Identity
    Quaternion qEnd(0.0f, 1.0f, 0.0f, 0.0f);   // 180 degree rotation
    Quaternion slerped = Quaternion::slerp(qStart, qEnd, 0.5f);

    EXPECT_GT(slerped.y, 0.0f); // Should be halfway between
    EXPECT_LT(slerped.y, 1.0f);

    // Test angle calculation
    float angle = Quaternion::angle(q1, q2);
    EXPECT_GT(angle, 0.0f);
    EXPECT_LT(angle, 3.14159f); // Less than 180 degrees

    // Test from Euler angles
    Quaternion eulerQuat = Quaternion::fromEulerAngles(1.57f, 0.0f, 0.0f); // 90 degrees around X
    EXPECT_NEAR(eulerQuat.x, 0.707f, 0.001f); // sin(45) ≈ 0.707
    EXPECT_FLOAT_EQ(eulerQuat.y, 0.0f);
    EXPECT_FLOAT_EQ(eulerQuat.z, 0.0f);
    EXPECT_NEAR(eulerQuat.w, 0.707f, 0.001f);
}

/**
 * @brief Test polynomial operations
 */
TEST_F(MathSystemsTest, PolynomialOperations) {
    // Test polynomial creation
    std::vector<float> coeffs = {1.0f, 2.0f, 3.0f}; // 3x² + 2x + 1
    Polynomial poly(coeffs);

    // Test evaluation
    float result1 = poly.evaluate(2.0f); // 3*(2)² + 2*(2) + 1 = 12 + 4 + 1 = 17
    EXPECT_FLOAT_EQ(result1, 17.0f);

    float result2 = poly.evaluate(0.0f); // 1
    EXPECT_FLOAT_EQ(result2, 1.0f);

    // Test derivative
    Polynomial derivative = poly.derivative();
    float derivResult = derivative.evaluate(2.0f); // 6x + 2 at x=2 = 14
    EXPECT_FLOAT_EQ(derivResult, 14.0f);

    // Test addition
    std::vector<float> coeffs2 = {1.0f, 1.0f}; // x + 1
    Polynomial poly2(coeffs2);
    Polynomial sum = poly + poly2;

    float sumResult = sum.evaluate(1.0f); // (3+1)x² + (2+1)x + (1+1) = 4 + 3 + 2 = 9
    EXPECT_FLOAT_EQ(sumResult, 9.0f);

    // Test multiplication
    Polynomial product = poly * poly2;
    float productResult = product.evaluate(1.0f); // (3x² + 2x + 1)(x + 1) = 3x³ + 5x² + 3x + 1
    EXPECT_FLOAT_EQ(productResult, 12.0f);

    // Test root finding (quadratic formula)
    std::vector<float> quadCoeffs = {-2.0f, 3.0f, -1.0f}; // -x² + 3x - 2 = 0
    Polynomial quad(quadCoeffs);

    // Roots should be x = 1 and x = 2
    float root1 = NumericalMethods::newtonRaphson(quad, 0.5f, 0.001f, 100);
    float root2 = NumericalMethods::newtonRaphson(quad, 2.5f, 0.001f, 100);

    EXPECT_NEAR(root1, 1.0f, 0.1f);
    EXPECT_NEAR(root2, 2.0f, 0.1f);
}

/**
 * @brief Test numerical methods
 */
TEST_F(MathSystemsTest, NumericalMethods) {
    // Test Newton-Raphson method
    auto func = [](float x) { return x * x - 4.0f; }; // Root at x = 2
    auto deriv = [](float x) { return 2.0f * x; };

    float root = NumericalMethods::newtonRaphson(func, deriv, 1.0f, 0.001f, 100);
    EXPECT_NEAR(root, 2.0f, 0.01f);

    // Test bisection method
    float bisectionRoot = NumericalMethods::bisection(func, 1.0f, 3.0f, 0.001f, 100);
    EXPECT_NEAR(bisectionRoot, 2.0f, 0.01f);

    // Test numerical integration
    auto integrand = [](float x) { return x * x; }; // ∫x² dx = x³/3
    float integral = NumericalMethods::simpsonIntegration(integrand, 0.0f, 2.0f, 1000);
    EXPECT_NEAR(integral, 8.0f / 3.0f, 0.01f); // (2)³/3 = 8/3

    // Test numerical differentiation
    auto differentiable = [](float x) { return x * x * x; }; // 3x²
    float derivative = NumericalMethods::numericalDerivative(differentiable, 2.0f, 0.001f);
    EXPECT_NEAR(derivative, 12.0f, 0.1f); // 3*(2)² = 12

    // Test linear system solving
    Matrix4 A;
    A.m[0] = 2.0f; A.m[1] = 1.0f;
    A.m[4] = 1.0f; A.m[5] = 2.0f;

    Vector4 b(3.0f, 3.0f, 0.0f, 0.0f);

    Vector4 solution = NumericalMethods::solveLinearSystem(A, b);
    // Should solve: 2x + y = 3, x + 2y = 3
    EXPECT_NEAR(solution.x, 1.0f, 0.01f);
    EXPECT_NEAR(solution.y, 1.0f, 0.01f);
}

/**
 * @brief Test math performance
 */
TEST_F(MathSystemsTest, Performance) {
    const int numIterations = 10000;

    // Measure vector operations performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        Vector3 v1(static_cast<float>(i), static_cast<float>(i + 1), static_cast<float>(i + 2));
        Vector3 v2(static_cast<float>(i + 3), static_cast<float>(i + 4), static_cast<float>(i + 5));

        Vector3 sum = v1 + v2;
        Vector3 cross = Vector3::cross(v1, v2);
        float dot = Vector3::dot(v1, v2);
        Vector3 normalized = v1.normalized();

        // Matrix operations
        Matrix4 mat1, mat2;
        Matrix4 product = mat1 * mat2;

        // Quaternion operations
        Quaternion q1, q2;
        Quaternion qProduct = q1 * q2;
        Matrix4 qMatrix = q1.toMatrix4();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numIterations << " math operations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 50ms for 10k operations)
    EXPECT_LT(duration.count(), 50000);
}

/**
 * @brief Test math edge cases
 */
TEST_F(MathSystemsTest, EdgeCases) {
    // Test zero vectors
    Vector3 zeroVec(0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(zeroVec.magnitude(), 0.0f);

    Vector3 normalizedZero = zeroVec.normalized();
    // Should handle gracefully (return zero vector)

    // Test very small numbers
    Vector3 tinyVec(1e-10f, 1e-10f, 1e-10f);
    float tinyMagnitude = tinyVec.magnitude();
    EXPECT_GT(tinyMagnitude, 0.0f);

    // Test very large numbers
    Vector3 hugeVec(1e10f, 1e10f, 1e10f);
    float hugeMagnitude = hugeVec.magnitude();
    EXPECT_LT(hugeMagnitude, 1e11f);

    // Test matrix singularity
    Matrix4 singular;
    // Make it singular (determinant = 0)
    singular.m[0] = 1.0f; singular.m[1] = 2.0f;
    singular.m[4] = 2.0f; singular.m[5] = 4.0f;

    float det = singular.determinant();
    EXPECT_NEAR(det, 0.0f, 0.001f);

    // Test quaternion normalization with zero quaternion
    Quaternion zeroQuat(0.0f, 0.0f, 0.0f, 0.0f);
    Quaternion normalizedZero = zeroQuat.normalized();
    // Should handle gracefully
}

/**
 * @brief Test math concurrent operations
 */
TEST_F(MathSystemsTest, ConcurrentOperations) {
    const int numThreads = 8;
    const int operationsPerThread = 1000;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads performing math operations
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&successCount, t]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                // Perform various math operations
                Vector3 v1(static_cast<float>(t), static_cast<float>(i), 1.0f);
                Vector3 v2(1.0f, 2.0f, 3.0f);

                Vector3 sum = v1 + v2;
                Vector3 cross = Vector3::cross(v1, v2);
                float dot = Vector3::dot(v1, v2);

                Matrix4 mat;
                Vector4 transformed = mat * Vector4(v1.x, v1.y, v1.z, 1.0f);

                Quaternion q = Quaternion::fromEulerAngles(
                    static_cast<float>(i) * 0.01f,
                    static_cast<float>(t) * 0.01f,
                    0.0f);

                if (sum.magnitude() > 0.0f && q.magnitude() > 0.0f) {
                    successCount++;
                }
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify concurrent operations worked
    EXPECT_EQ(successCount.load(), numThreads * operationsPerThread);
}

/**
 * @brief Test math accuracy and precision
 */
TEST_F(MathSystemsTest, AccuracyAndPrecision) {
    // Test floating point precision
    Vector3 preciseVec(1.0f, 1.0f, 1.0f);
    Vector3 normalizedPrecise = preciseVec.normalized();

    // Magnitude should be exactly 1.0f within floating point precision
    EXPECT_NEAR(normalizedPrecise.magnitude(), 1.0f, 1e-6f);

    // Test matrix inverse accuracy
    Matrix4 testMatrix;
    testMatrix.m[0] = 2.0f; testMatrix.m[5] = 3.0f; testMatrix.m[10] = 1.0f; testMatrix.m[15] = 1.0f;
    testMatrix.m[12] = 5.0f; testMatrix.m[13] = 7.0f; testMatrix.m[14] = 9.0f;

    Matrix4 inverse = testMatrix.inverse();
    Matrix4 identityCheck = testMatrix * inverse;

    // Should be close to identity matrix
    EXPECT_NEAR(identityCheck.m[0], 1.0f, 1e-5f);
    EXPECT_NEAR(identityCheck.m[5], 1.0f, 1e-5f);
    EXPECT_NEAR(identityCheck.m[10], 1.0f, 1e-5f);
    EXPECT_NEAR(identityCheck.m[15], 1.0f, 1e-5f);

    // Test quaternion accuracy
    Quaternion testQuat = Quaternion::fromEulerAngles(1.5708f, 0.0f, 0.0f); // ~90 degrees
    Matrix4 quatMatrix = testQuat.toMatrix4();

    // Should be close to 90-degree rotation matrix
    EXPECT_NEAR(quatMatrix.m[0], 0.0f, 1e-5f);
    EXPECT_NEAR(quatMatrix.m[2], -1.0f, 1e-5f);
    EXPECT_NEAR(quatMatrix.m[8], 1.0f, 1e-5f);
    EXPECT_NEAR(quatMatrix.m[10], 0.0f, 1e-5f);
}

/**
 * @brief Test math utility functions
 */
TEST_F(MathSystemsTest, UtilityFunctions) {
    // Test clamp function
    float clamped = std::clamp(5.0f, 0.0f, 3.0f);
    EXPECT_FLOAT_EQ(clamped, 3.0f);

    clamped = std::clamp(-1.0f, 0.0f, 3.0f);
    EXPECT_FLOAT_EQ(clamped, 0.0f);

    // Test lerp function
    float lerped = std::lerp(0.0f, 10.0f, 0.5f);
    EXPECT_FLOAT_EQ(lerped, 5.0f);

    // Test smoothstep function
    float smoothstepped = std::smoothstep(0.0f, 1.0f, 0.5f);
    EXPECT_FLOAT_EQ(smoothstepped, 0.5f);

    smoothstepped = std::smoothstep(0.0f, 1.0f, 0.0f);
    EXPECT_FLOAT_EQ(smoothstepped, 0.0f);

    smoothstepped = std::smoothstep(0.0f, 1.0f, 1.0f);
    EXPECT_FLOAT_EQ(smoothstepped, 1.0f);

    // Test angle conversions
    float radians = deg2rad(90.0f);
    EXPECT_FLOAT_EQ(radians, 1.5708f);

    float degrees = rad2deg(1.5708f);
    EXPECT_FLOAT_EQ(degrees, 90.0f);

    // Test power functions
    float pow2 = pow_n(2.0f, 3); // 2^3 = 8
    EXPECT_FLOAT_EQ(pow2, 8.0f);

    float sqrt2 = sqrt_approx(4.0f); // sqrt(4) ≈ 2
    EXPECT_NEAR(sqrt2, 2.0f, 0.1f);
}

} // namespace Tests
} // namespace FoundryEngine
