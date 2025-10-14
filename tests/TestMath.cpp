#include <gtest/gtest.h>
#include "GameEngine/math/Vector3.h"
#include "GameEngine/math/Matrix4.h"

using namespace FoundryEngine;

class MathTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures
    }
    
    void TearDown() override {
        // Clean up test fixtures
    }
};

TEST_F(MathTest, Vector3BasicOperations) {
    Vector3 v1(1.0f, 2.0f, 3.0f);
    Vector3 v2(4.0f, 5.0f, 6.0f);
    
    // Test addition
    Vector3 sum = v1 + v2;
    EXPECT_FLOAT_EQ(sum.x, 5.0f);
    EXPECT_FLOAT_EQ(sum.y, 7.0f);
    EXPECT_FLOAT_EQ(sum.z, 9.0f);
    
    // Test subtraction
    Vector3 diff = v2 - v1;
    EXPECT_FLOAT_EQ(diff.x, 3.0f);
    EXPECT_FLOAT_EQ(diff.y, 3.0f);
    EXPECT_FLOAT_EQ(diff.z, 3.0f);
    
    // Test scalar multiplication
    Vector3 scaled = v1 * 2.0f;
    EXPECT_FLOAT_EQ(scaled.x, 2.0f);
    EXPECT_FLOAT_EQ(scaled.y, 4.0f);
    EXPECT_FLOAT_EQ(scaled.z, 6.0f);
}

TEST_F(MathTest, Vector3Magnitude) {
    Vector3 v(3.0f, 4.0f, 0.0f);
    
    EXPECT_FLOAT_EQ(v.magnitude(), 5.0f);
    EXPECT_FLOAT_EQ(v.magnitudeSq(), 25.0f);
}

TEST_F(MathTest, Vector3Normalization) {
    Vector3 v(3.0f, 4.0f, 0.0f);
    Vector3 normalized = v.normalized();
    
    EXPECT_FLOAT_EQ(normalized.magnitude(), 1.0f);
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);
    EXPECT_FLOAT_EQ(normalized.z, 0.0f);
}

TEST_F(MathTest, Vector3DotProduct) {
    Vector3 v1(1.0f, 2.0f, 3.0f);
    Vector3 v2(4.0f, 5.0f, 6.0f);
    
    float dot = v1.dot(v2);
    EXPECT_FLOAT_EQ(dot, 32.0f); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
}

TEST_F(MathTest, Vector3CrossProduct) {
    Vector3 v1(1.0f, 0.0f, 0.0f);
    Vector3 v2(0.0f, 1.0f, 0.0f);
    
    Vector3 cross = v1.cross(v2);
    EXPECT_FLOAT_EQ(cross.x, 0.0f);
    EXPECT_FLOAT_EQ(cross.y, 0.0f);
    EXPECT_FLOAT_EQ(cross.z, 1.0f);
}

TEST_F(MathTest, Vector3Lerp) {
    Vector3 v1(0.0f, 0.0f, 0.0f);
    Vector3 v2(10.0f, 10.0f, 10.0f);
    
    Vector3 lerped = v1.lerp(v2, 0.5f);
    EXPECT_FLOAT_EQ(lerped.x, 5.0f);
    EXPECT_FLOAT_EQ(lerped.y, 5.0f);
    EXPECT_FLOAT_EQ(lerped.z, 5.0f);
}

TEST_F(MathTest, Matrix4Identity) {
    Matrix4 identity = Matrix4::identity();
    
    // Check diagonal elements
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i == j) {
                EXPECT_FLOAT_EQ(identity.m[i][j], 1.0f);
            } else {
                EXPECT_FLOAT_EQ(identity.m[i][j], 0.0f);
            }
        }
    }
}

TEST_F(MathTest, Matrix4Multiplication) {
    Matrix4 m1 = Matrix4::identity();
    Matrix4 m2 = Matrix4::identity();
    
    Matrix4 result = m1 * m2;
    
    // Identity * Identity should be Identity
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i == j) {
                EXPECT_FLOAT_EQ(result.m[i][j], 1.0f);
            } else {
                EXPECT_FLOAT_EQ(result.m[i][j], 0.0f);
            }
        }
    }
}

TEST_F(MathTest, Matrix4VectorMultiplication) {
    Matrix4 m = Matrix4::identity();
    Vector3 v(1.0f, 2.0f, 3.0f);
    
    Vector3 result = m * v;
    
    // Identity matrix should not change the vector
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
}

TEST_F(MathTest, Matrix4Translation) {
    Matrix4 m = Matrix4::identity();
    Vector3 translation(5.0f, 10.0f, 15.0f);
    
    m.translate(translation);
    
    // Check translation components
    EXPECT_FLOAT_EQ(m.m[0][3], 5.0f);
    EXPECT_FLOAT_EQ(m.m[1][3], 10.0f);
    EXPECT_FLOAT_EQ(m.m[2][3], 15.0f);
}

TEST_F(MathTest, Matrix4Scaling) {
    Matrix4 m = Matrix4::identity();
    Vector3 scale(2.0f, 3.0f, 4.0f);
    
    m.scale(scale);
    
    // Check scaling components
    EXPECT_FLOAT_EQ(m.m[0][0], 2.0f);
    EXPECT_FLOAT_EQ(m.m[1][1], 3.0f);
    EXPECT_FLOAT_EQ(m.m[2][2], 4.0f);
}
