#include "gtest/gtest.h"
#include "GameEngine/core/SerializationSystem.h"
#include "GameEngine/math/Vector3.h"
#include "GameEngine/math/Quaternion.h"
#include "GameEngine/math/Matrix4.h"
#include <sstream>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Serialization System tests
 */
class SerializationSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test data
        testVector3 = Vector3(1.0f, 2.0f, 3.0f);
        testQuaternion = Quaternion(0.0f, 1.0f, 0.0f, 1.0f);
        testMatrix4 = Matrix4();
        testMatrix4.m[0] = 1.0f; testMatrix4.m[5] = 1.0f; // Identity matrix
        testMatrix4.m[10] = 1.0f; testMatrix4.m[15] = 1.0f;
    }

    Vector3 testVector3;
    Quaternion testQuaternion;
    Matrix4 testMatrix4;
};

/**
 * @brief Test serialization buffer basic operations
 */
TEST_F(SerializationSystemTest, SerializationBufferBasics) {
    SerializationBuffer buffer;

    // Test initial state
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
    EXPECT_EQ(buffer.remaining(), 0);

    // Test writing data
    buffer.writeBool(true);
    buffer.writeInt32(42);
    buffer.writeFloat(3.14f);
    buffer.writeString("test string");

    EXPECT_FALSE(buffer.empty());
    EXPECT_GT(buffer.size(), 0);

    // Test reading data
    buffer.resetReadPosition();
    EXPECT_TRUE(buffer.readBool());
    EXPECT_EQ(buffer.readInt32(), 42);
    EXPECT_FLOAT_EQ(buffer.readFloat(), 3.14f);
    EXPECT_EQ(buffer.readString(), "test string");
}

/**
 * @brief Test serialization buffer type safety
 */
TEST_F(SerializationSystemTest, TypeSafety) {
    SerializationBuffer buffer;

    // Test typed writing and reading
    buffer.writeTyped(testVector3);
    buffer.writeTyped(testQuaternion);
    buffer.writeTyped(testMatrix4);

    // Reset for reading
    buffer.resetReadPosition();

    // Test typed reading
    auto resultVec3 = buffer.readTyped<Vector3>();
    EXPECT_TRUE(resultVec3.success);
    EXPECT_EQ(resultVec3.data.x, testVector3.x);
    EXPECT_EQ(resultVec3.data.y, testVector3.y);
    EXPECT_EQ(resultVec3.data.z, testVector3.z);

    auto resultQuat = buffer.readTyped<Quaternion>();
    EXPECT_TRUE(resultQuat.success);
    EXPECT_EQ(resultQuat.data.x, testQuaternion.x);
    EXPECT_EQ(resultQuat.data.y, testQuaternion.y);
    EXPECT_EQ(resultQuat.data.z, testQuaternion.z);
    EXPECT_EQ(resultQuat.data.w, testQuaternion.w);

    auto resultMat4 = buffer.readTyped<Matrix4>();
    EXPECT_TRUE(resultMat4.success);
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(resultMat4.data.m[i], testMatrix4.m[i]);
    }
}

/**
 * @brief Test serialization buffer error handling
 */
TEST_F(SerializationSystemTest, ErrorHandling) {
    SerializationBuffer buffer;

    // Write minimal data
    buffer.writeInt32(123);

    // Try to read more data than available
    buffer.resetReadPosition();

    EXPECT_EQ(buffer.readInt32(), 123);

    // This should handle gracefully (return default values)
    int32_t result = buffer.readInt32(); // Reading beyond buffer
    // The exact behavior depends on implementation, but should not crash
}

/**
 * @brief Test serialization results
 */
TEST_F(SerializationSystemTest, SerializationResults) {
    SerializationResult<int> successResult(42, true);
    EXPECT_TRUE(successResult);
    EXPECT_EQ(successResult.data, 42);
    EXPECT_TRUE(successResult.success);

    SerializationResult<int> errorResult(0, "Test error");
    EXPECT_FALSE(errorResult);
    EXPECT_EQ(errorResult.errorMessage, "Test error");

    SerializationResult<float> defaultResult;
    EXPECT_FALSE(defaultResult);
}

/**
 * @brief Test serialization stream interface
 */
TEST_F(SerializationSystemTest, SerializationStream) {
    SerializationBuffer buffer;
    SerializationStream stream(buffer);

    // Test stream writing
    stream << true << 42 << 3.14f << "test" << testVector3 << testQuaternion;

    // Test stream reading
    buffer.resetReadPosition();
    SerializationStream readStream(buffer);

    bool boolVal;
    int intVal;
    float floatVal;
    std::string stringVal;
    Vector3 vec3Val;
    Quaternion quatVal;

    readStream >> boolVal >> intVal >> floatVal >> stringVal >> vec3Val >> quatVal;

    EXPECT_TRUE(boolVal);
    EXPECT_EQ(intVal, 42);
    EXPECT_FLOAT_EQ(floatVal, 3.14f);
    EXPECT_EQ(stringVal, "test");
    EXPECT_EQ(vec3Val.x, testVector3.x);
    EXPECT_EQ(vec3Val.y, testVector3.y);
    EXPECT_EQ(vec3Val.z, testVector3.z);
    EXPECT_EQ(quatVal.x, testQuaternion.x);
    EXPECT_EQ(quatVal.y, testQuaternion.y);
    EXPECT_EQ(quatVal.z, testQuaternion.z);
    EXPECT_EQ(quatVal.w, testQuaternion.w);
}

/**
 * @brief Test complex data structure serialization
 */
TEST_F(SerializationSystemTest, ComplexSerialization) {
    // Define a complex test structure
    struct ComplexData {
        int id;
        std::string name;
        Vector3 position;
        Quaternion rotation;
        std::vector<float> values;
        bool enabled;
    };

    ComplexData original;
    original.id = 123;
    original.name = "Complex Test Object";
    original.position = Vector3(1.0f, 2.0f, 3.0f);
    original.rotation = Quaternion(0.1f, 0.2f, 0.3f, 1.0f);
    original.values = {1.1f, 2.2f, 3.3f, 4.4f};
    original.enabled = true;

    // Serialize
    SerializationBuffer buffer;
    buffer.writeInt32(original.id);
    buffer.writeString(original.name);
    buffer.writeVector3(original.position);
    buffer.writeQuaternion(original.rotation);
    buffer.writeUint32(static_cast<uint32_t>(original.values.size()));
    for (float value : original.values) {
        buffer.writeFloat(value);
    }
    buffer.writeBool(original.enabled);

    // Deserialize
    buffer.resetReadPosition();
    ComplexData deserialized;
    deserialized.id = buffer.readInt32();
    deserialized.name = buffer.readString();
    deserialized.position = buffer.readVector3();
    deserialized.rotation = buffer.readQuaternion();
    uint32_t valueCount = buffer.readUint32();
    deserialized.values.resize(valueCount);
    for (uint32_t i = 0; i < valueCount; ++i) {
        deserialized.values[i] = buffer.readFloat();
    }
    deserialized.enabled = buffer.readBool();

    // Verify
    EXPECT_EQ(deserialized.id, original.id);
    EXPECT_EQ(deserialized.name, original.name);
    EXPECT_EQ(deserialized.position.x, original.position.x);
    EXPECT_EQ(deserialized.position.y, original.position.y);
    EXPECT_EQ(deserialized.position.z, original.position.z);
    EXPECT_EQ(deserialized.rotation.x, original.rotation.x);
    EXPECT_EQ(deserialized.rotation.y, original.rotation.y);
    EXPECT_EQ(deserialized.rotation.z, original.rotation.z);
    EXPECT_EQ(deserialized.rotation.w, original.rotation.w);
    EXPECT_EQ(deserialized.values.size(), original.values.size());
    for (size_t i = 0; i < original.values.size(); ++i) {
        EXPECT_FLOAT_EQ(deserialized.values[i], original.values[i]);
    }
    EXPECT_EQ(deserialized.enabled, original.enabled);
}

/**
 * @brief Test serialization performance
 */
TEST_F(SerializationSystemTest, Performance) {
    const int numIterations = 1000;

    // Measure serialization performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        SerializationBuffer buffer;
        buffer.writeVector3(testVector3);
        buffer.writeQuaternion(testQuaternion);
        buffer.writeMatrix4(testMatrix4);
        buffer.writeString("Performance test string");

        buffer.resetReadPosition();
        buffer.readVector3();
        buffer.readQuaternion();
        buffer.readMatrix4();
        buffer.readString();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Serialized and deserialized " << numIterations
              << " objects in " << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 10ms for 1000 operations)
    EXPECT_LT(duration.count(), 10000);
}

/**
 * @brief Test serialization buffer edge cases
 */
TEST_F(SerializationSystemTest, EdgeCases) {
    SerializationBuffer buffer;

    // Test empty string serialization
    buffer.writeString("");
    buffer.resetReadPosition();
    EXPECT_EQ(buffer.readString(), "");

    // Test large data handling
    std::string largeString(10000, 'A');
    buffer.writeString(largeString);
    buffer.resetReadPosition();
    EXPECT_EQ(buffer.readString(), largeString);

    // Test position management
    buffer.writeInt32(1);
    buffer.writeInt32(2);
    buffer.writeInt32(3);

    buffer.setReadPosition(4); // Skip first int32
    EXPECT_EQ(buffer.readInt32(), 2);
    EXPECT_EQ(buffer.readInt32(), 3);

    // Test buffer resizing
    SerializationBuffer growingBuffer;
    for (int i = 0; i < 100; ++i) {
        growingBuffer.writeInt32(i);
    }
    EXPECT_GE(growingBuffer.size(), 400); // 100 * sizeof(int32_t)
}

/**
 * @brief Test serialization with custom types
 */
TEST_F(SerializationSystemTest, CustomTypes) {
    // Define custom serializable type
    struct CustomType {
        double value;
        std::string description;
    };

    SerializationBuffer buffer;

    // Write custom type manually
    CustomType original{3.14159, "Custom test type"};
    buffer.writeDouble(original.value);
    buffer.writeString(original.description);

    // Read custom type manually
    buffer.resetReadPosition();
    CustomType deserialized;
    deserialized.value = buffer.readDouble();
    deserialized.description = buffer.readString();

    // Verify
    EXPECT_DOUBLE_EQ(deserialized.value, original.value);
    EXPECT_EQ(deserialized.description, original.description);
}

/**
 * @brief Test serialization buffer with binary data
 */
TEST_F(SerializationSystemTest, BinaryData) {
    // Test with binary data that might contain null bytes
    std::vector<std::byte> binaryData = {
        std::byte{0x00}, std::byte{0x01}, std::byte{0x02}, std::byte{0xFF},
        std::byte{0xFE}, std::byte{0xFD}, std::byte{0x00}, std::byte{0x80}
    };

    SerializationBuffer buffer;
    for (std::byte b : binaryData) {
        buffer.writeUint8(static_cast<uint8_t>(b));
    }

    buffer.resetReadPosition();
    std::vector<std::byte> readData;
    for (size_t i = 0; i < binaryData.size(); ++i) {
        readData.push_back(std::byte{buffer.readUint8()});
    }

    EXPECT_EQ(binaryData.size(), readData.size());
    for (size_t i = 0; i < binaryData.size(); ++i) {
        EXPECT_EQ(binaryData[i], readData[i]);
    }
}

/**
 * @brief Test serialization buffer stress test
 */
TEST_F(SerializationSystemTest, StressTest) {
    SerializationBuffer buffer;

    // Write a large amount of varied data
    const int numElements = 1000;

    for (int i = 0; i < numElements; ++i) {
        buffer.writeInt32(i);
        buffer.writeFloat(static_cast<float>(i) * 1.1f);
        buffer.writeBool(i % 2 == 0);
        buffer.writeString("String " + std::to_string(i));
    }

    // Read back and verify
    buffer.resetReadPosition();

    for (int i = 0; i < numElements; ++i) {
        EXPECT_EQ(buffer.readInt32(), i);
        EXPECT_FLOAT_EQ(buffer.readFloat(), static_cast<float>(i) * 1.1f);
        EXPECT_EQ(buffer.readBool(), i % 2 == 0);
        EXPECT_EQ(buffer.readString(), "String " + std::to_string(i));
    }

    // Should have read all data
    EXPECT_EQ(buffer.remaining(), 0);
}

} // namespace Tests
} // namespace FoundryEngine
