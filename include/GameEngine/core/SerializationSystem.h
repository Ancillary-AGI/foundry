#pragma once

#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include "../math/Vector3.h"
#include "../math/Quaternion.h"
#include "../math/Matrix4.h"

namespace FoundryEngine {

/**
 * @brief Serialization result with type safety
 */
template<typename T>
struct SerializationResult {
    T data;
    bool success;
    std::string errorMessage;

    SerializationResult() : success(false) {}
    SerializationResult(T value, bool ok = true) : data(value), success(ok) {}
    SerializationResult(T value, const std::string& error) : data(value), success(false), errorMessage(error) {}

    explicit operator bool() const { return success; }
};

/**
 * @brief Type-safe serialization buffer
 */
class SerializationBuffer {
public:
    /**
     * @brief Construct empty buffer
     */
    SerializationBuffer() = default;

    /**
     * @brief Construct buffer with initial data
     * @param data Initial data pointer
     * @param size Size of initial data
     */
    SerializationBuffer(const void* data, size_t size) : buffer_(static_cast<const std::byte*>(data), static_cast<const std::byte*>(data) + size) {}

    /**
     * @brief Construct buffer from vector
     * @param data Vector of bytes
     */
    explicit SerializationBuffer(const std::vector<std::byte>& data) : buffer_(data) {}

    /**
     * @brief Get buffer data as void pointer (for compatibility)
     * @return Const void pointer to buffer data
     */
    const void* data() const {
        return buffer_.data();
    }

    /**
     * @brief Get buffer size
     * @return Size of buffer in bytes
     */
    size_t size() const {
        return buffer_.size();
    }

    /**
     * @brief Check if buffer is empty
     * @return true if buffer contains no data
     */
    bool empty() const {
        return buffer_.empty();
    }

    /**
     * @brief Clear buffer contents
     */
    void clear() {
        buffer_.clear();
        readPosition_ = 0;
    }

    /**
     * @brief Reserve buffer capacity
     * @param capacity New capacity in bytes
     */
    void reserve(size_t capacity) {
        buffer_.reserve(capacity);
    }

    /**
     * @brief Get remaining bytes available for reading
     * @return Number of bytes left to read
     */
    size_t remaining() const {
        return buffer_.size() - readPosition_;
    }

    // Write operations (for serialization)
    void writeBool(bool value);
    void writeInt8(int8_t value);
    void writeInt16(int16_t value);
    void writeInt32(int32_t value);
    void writeInt64(int64_t value);
    void writeUint8(uint8_t value);
    void writeUint16(uint16_t value);
    void writeUint32(uint32_t value);
    void writeUint64(uint64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeString(const std::string& value);
    void writeVector3(const Vector3& value);
    void writeQuaternion(const Quaternion& value);
    void writeMatrix4(const Matrix4& value);

    template<typename T>
    void writeTyped(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable for serialization");
        writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(T));
    }

    // Read operations (for deserialization)
    bool readBool();
    int8_t readInt8();
    int16_t readInt16();
    int32_t readInt32();
    int64_t readInt64();
    uint8_t readUint8();
    uint16_t readUint16();
    uint32_t readUint32();
    uint64_t readUint64();
    float readFloat();
    double readDouble();
    std::string readString();
    Vector3 readVector3();
    Quaternion readQuaternion();
    Matrix4 readMatrix4();

    template<typename T>
    SerializationResult<T> readTyped() {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable for deserialization");

        if (remaining() < sizeof(T)) {
            return SerializationResult<T>(T{}, "Insufficient data in buffer");
        }

        T value;
        std::memcpy(&value, buffer_.data() + readPosition_, sizeof(T));
        readPosition_ += sizeof(T);

        return SerializationResult<T>(value, true);
    }

    // Utility functions
    void resetReadPosition();
    void setReadPosition(size_t position);
    size_t getReadPosition() const { return readPosition_; }

private:
    void writeBytes(const std::byte* data, size_t size);
    void ensureSpace(size_t needed);

    std::vector<std::byte> buffer_;
    size_t readPosition_ = 0;
};

/**
 * @brief Type-safe serialization interface
 */
class Serializable {
public:
    virtual ~Serializable() = default;

    /**
     * @brief Serialize object to buffer
     * @param buffer Buffer to serialize into
     * @return true if serialization succeeded
     */
    virtual bool serialize(SerializationBuffer& buffer) const = 0;

    /**
     * @brief Deserialize object from buffer
     * @param buffer Buffer to deserialize from
     * @return true if deserialization succeeded
     */
    virtual bool deserialize(const SerializationBuffer& buffer) = 0;

    /**
     * @brief Get serialized size estimate
     * @return Estimated size in bytes needed for serialization
     */
    virtual size_t getSerializedSize() const = 0;

    /**
     * @brief Get object type name for serialization
     * @return Type name as string
     */
    virtual std::string getTypeName() const = 0;
};

/**
 * @brief Type-safe component serialization wrapper
 */
template<typename T>
class ComponentSerializer {
public:
    /**
     * @brief Serialize component to type-safe buffer
     * @param component Component to serialize
     * @return Serialization buffer with component data
     */
    static SerializationBuffer serialize(const T& component) {
        static_assert(std::is_base_of_v<Serializable, T>, "Component must inherit from Serializable");

        SerializationBuffer buffer;
        buffer.reserve(component.getSerializedSize());

        if (!component.serialize(buffer)) {
            // Handle serialization failure
            buffer.clear();
        }

        return buffer;
    }

    /**
     * @brief Deserialize component from type-safe buffer
     * @param buffer Buffer containing serialized data
     * @param component Component to deserialize into
     * @return true if deserialization succeeded
     */
    static bool deserialize(const SerializationBuffer& buffer, T& component) {
        static_assert(std::is_base_of_v<Serializable, T>, "Component must inherit from Serializable");

        size_t originalPosition = buffer.getReadPosition();
        bool success = component.deserialize(buffer);

        if (!success) {
            // Reset read position on failure
            const_cast<SerializationBuffer&>(buffer).setReadPosition(originalPosition);
        }

        return success;
    }

    /**
     * @brief Create component from serialized data
     * @param buffer Buffer containing serialized data
     * @return Unique pointer to deserialized component or nullptr on failure
     */
    static std::unique_ptr<T> createFromBuffer(const SerializationBuffer& buffer) {
        auto component = std::make_unique<T>();
        if (component && deserialize(buffer, *component)) {
            return component;
        }
        return nullptr;
    }
};

/**
 * @brief Type-safe serialization for built-in types
 */
namespace Serialization {

// Boolean serialization
SerializationResult<bool> serialize(bool value);
SerializationResult<bool> deserialize(const SerializationBuffer& buffer);

// Integer serialization
template<typename T>
SerializationResult<T> serialize(T value) {
    static_assert(std::is_integral_v<T> && std::is_signed, "Type must be a signed integer type");
    return SerializationResult<T>(value, true);
}

template<typename T>
SerializationResult<T> deserialize(const SerializationBuffer& buffer) {
    static_assert(std::is_integral_v<T> && std::is_signed, "Type must be a signed integer type");
    return buffer.readTyped<T>();
}

// Unsigned integer serialization
template<typename T>
SerializationResult<T> serializeUnsigned(T value) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>, "Type must be an unsigned integer type");
    return SerializationResult<T>(value, true);
}

template<typename T>
SerializationResult<T> deserializeUnsigned(const SerializationBuffer& buffer) {
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>, "Type must be an unsigned integer type");
    return buffer.readTyped<T>();
}

// Floating point serialization
template<typename T>
SerializationResult<T> serialize(T value) {
    static_assert(std::is_floating_point_v<T>, "Type must be a floating point type");
    return SerializationResult<T>(value, true);
}

template<typename T>
SerializationResult<T> deserialize(const SerializationBuffer& buffer) {
    static_assert(std::is_floating_point_v<T>, "Type must be a floating point type");
    return buffer.readTyped<T>();
}

// String serialization
SerializationResult<std::string> serialize(const std::string& value);
SerializationResult<std::string> deserialize(const SerializationBuffer& buffer);

// Vector3 serialization
SerializationResult<Vector3> serialize(const Vector3& value);
SerializationResult<Vector3> deserialize(const SerializationBuffer& buffer);

// Quaternion serialization
SerializationResult<Quaternion> serialize(const Quaternion& value);
SerializationResult<Quaternion> deserialize(const SerializationBuffer& buffer);

// Matrix4 serialization
SerializationResult<Matrix4> serialize(const Matrix4& value);
SerializationResult<Matrix4> deserialize(const SerializationBuffer& buffer);

} // namespace Serialization

/**
 * @brief Serialization stream for complex data structures
 */
class SerializationStream {
public:
    explicit SerializationStream(SerializationBuffer& buffer) : buffer_(buffer) {}

    // Stream-like interface for serialization
    SerializationStream& operator<<(bool value) { buffer_.writeBool(value); return *this; }
    SerializationStream& operator<<(int8_t value) { buffer_.writeInt8(value); return *this; }
    SerializationStream& operator<<(int16_t value) { buffer_.writeInt16(value); return *this; }
    SerializationStream& operator<<(int32_t value) { buffer_.writeInt32(value); return *this; }
    SerializationStream& operator<<(int64_t value) { buffer_.writeInt64(value); return *this; }
    SerializationStream& operator<<(uint8_t value) { buffer_.writeUint8(value); return *this; }
    SerializationStream& operator<<(uint16_t value) { buffer_.writeUint16(value); return *this; }
    SerializationStream& operator<<(uint32_t value) { buffer_.writeUint32(value); return *this; }
    SerializationStream& operator<<(uint64_t value) { buffer_.writeUint64(value); return *this; }
    SerializationStream& operator<<(float value) { buffer_.writeFloat(value); return *this; }
    SerializationStream& operator<<(double value) { buffer_.writeDouble(value); return *this; }
    SerializationStream& operator<<(const std::string& value) { buffer_.writeString(value); return *this; }
    SerializationStream& operator<<(const Vector3& value) { buffer_.writeVector3(value); return *this; }
    SerializationStream& operator<<(const Quaternion& value) { buffer_.writeQuaternion(value); return *this; }
    SerializationStream& operator<<(const Matrix4& value) { buffer_.writeMatrix4(value); return *this; }

    SerializationStream& operator>>(bool& value) { value = buffer_.readBool(); return *this; }
    SerializationStream& operator>>(int8_t& value) { value = buffer_.readInt8(); return *this; }
    SerializationStream& operator>>(int16_t& value) { value = buffer_.readInt16(); return *this; }
    SerializationStream& operator>>(int32_t& value) { value = buffer_.readInt32(); return *this; }
    SerializationStream& operator>>(int64_t& value) { value = buffer_.readInt64(); return *this; }
    SerializationStream& operator>>(uint8_t& value) { value = buffer_.readUint8(); return *this; }
    SerializationStream& operator>>(uint16_t& value) { value = buffer_.readUint16(); return *this; }
    SerializationStream& operator>>(uint32_t& value) { value = buffer_.readUint32(); return *this; }
    SerializationStream& operator>>(uint64_t& value) { value = buffer_.readUint64(); return *this; }
    SerializationStream& operator>>(float& value) { value = buffer_.readFloat(); return *this; }
    SerializationStream& operator>>(double& value) { value = buffer_.readDouble(); return *this; }
    SerializationStream& operator>>(std::string& value) { value = buffer_.readString(); return *this; }
    SerializationStream& operator>>(Vector3& value) { value = buffer_.readVector3(); return *this; }
    SerializationStream& operator>>(Quaternion& value) { value = buffer_.readQuaternion(); return *this; }
    SerializationStream& operator>>(Matrix4& value) { value = buffer_.readMatrix4(); return *this; }

private:
    SerializationBuffer& buffer_;
};

} // namespace FoundryEngine
