#include "GameEngine/core/SerializationSystem.h"
#include <cstring>
#include <algorithm>

namespace FoundryEngine {

// SerializationBuffer Implementation
void SerializationBuffer::writeBool(bool value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(bool));
}

void SerializationBuffer::writeInt8(int8_t value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(int8_t));
}

void SerializationBuffer::writeInt16(int16_t value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(int16_t));
}

void SerializationBuffer::writeInt32(int32_t value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(int32_t));
}

void SerializationBuffer::writeInt64(int64_t value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(int64_t));
}

void SerializationBuffer::writeUint8(uint8_t value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(uint8_t));
}

void SerializationBuffer::writeUint16(uint16_t value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(uint16_t));
}

void SerializationBuffer::writeUint32(uint32_t value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(uint32_t));
}

void SerializationBuffer::writeUint64(uint64_t value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(uint64_t));
}

void SerializationBuffer::writeFloat(float value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(float));
}

void SerializationBuffer::writeDouble(double value) {
    writeBytes(reinterpret_cast<const std::byte*>(&value), sizeof(double));
}

void SerializationBuffer::writeString(const std::string& value) {
    // Write string length first
    uint32_t length = static_cast<uint32_t>(value.length());
    writeUint32(length);

    // Write string data
    if (length > 0) {
        writeBytes(reinterpret_cast<const std::byte*>(value.c_str()), length);
    }
}

void SerializationBuffer::writeVector3(const Vector3& value) {
    writeFloat(value.x);
    writeFloat(value.y);
    writeFloat(value.z);
}

void SerializationBuffer::writeQuaternion(const Quaternion& value) {
    writeFloat(value.x);
    writeFloat(value.y);
    writeFloat(value.z);
    writeFloat(value.w);
}

void SerializationBuffer::writeMatrix4(const Matrix4& value) {
    for (int i = 0; i < 16; ++i) {
        writeFloat(value.m[i]);
    }
}

bool SerializationBuffer::readBool() {
    if (remaining() < sizeof(bool)) return false;
    bool value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(bool));
    readPosition_ += sizeof(bool);
    return value;
}

int8_t SerializationBuffer::readInt8() {
    if (remaining() < sizeof(int8_t)) return 0;
    int8_t value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(int8_t));
    readPosition_ += sizeof(int8_t);
    return value;
}

int16_t SerializationBuffer::readInt16() {
    if (remaining() < sizeof(int16_t)) return 0;
    int16_t value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(int16_t));
    readPosition_ += sizeof(int16_t);
    return value;
}

int32_t SerializationBuffer::readInt32() {
    if (remaining() < sizeof(int32_t)) return 0;
    int32_t value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(int32_t));
    readPosition_ += sizeof(int32_t);
    return value;
}

int64_t SerializationBuffer::readInt64() {
    if (remaining() < sizeof(int64_t)) return 0;
    int64_t value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(int64_t));
    readPosition_ += sizeof(int64_t);
    return value;
}

uint8_t SerializationBuffer::readUint8() {
    if (remaining() < sizeof(uint8_t)) return 0;
    uint8_t value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(uint8_t));
    readPosition_ += sizeof(uint8_t);
    return value;
}

uint16_t SerializationBuffer::readUint16() {
    if (remaining() < sizeof(uint16_t)) return 0;
    uint16_t value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(uint16_t));
    readPosition_ += sizeof(uint16_t);
    return value;
}

uint32_t SerializationBuffer::readUint32() {
    if (remaining() < sizeof(uint32_t)) return 0;
    uint32_t value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(uint32_t));
    readPosition_ += sizeof(uint32_t);
    return value;
}

uint64_t SerializationBuffer::readUint64() {
    if (remaining() < sizeof(uint64_t)) return 0;
    uint64_t value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(uint64_t));
    readPosition_ += sizeof(uint64_t);
    return value;
}

float SerializationBuffer::readFloat() {
    if (remaining() < sizeof(float)) return 0.0f;
    float value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(float));
    readPosition_ += sizeof(float);
    return value;
}

double SerializationBuffer::readDouble() {
    if (remaining() < sizeof(double)) return 0.0;
    double value;
    std::memcpy(&value, buffer_.data() + readPosition_, sizeof(double));
    readPosition_ += sizeof(double);
    return value;
}

std::string SerializationBuffer::readString() {
    uint32_t length = readUint32();
    if (length == 0) return "";

    if (remaining() < length) return "";

    std::string value(length, '\0');
    std::memcpy(&value[0], buffer_.data() + readPosition_, length);
    readPosition_ += length;

    return value;
}

Vector3 SerializationBuffer::readVector3() {
    float x = readFloat();
    float y = readFloat();
    float z = readFloat();
    return Vector3(x, y, z);
}

Quaternion SerializationBuffer::readQuaternion() {
    float x = readFloat();
    float y = readFloat();
    float z = readFloat();
    float w = readFloat();
    return Quaternion(x, y, z, w);
}

Matrix4 SerializationBuffer::readMatrix4() {
    Matrix4 matrix;
    for (int i = 0; i < 16; ++i) {
        matrix.m[i] = readFloat();
    }
    return matrix;
}

void SerializationBuffer::resetReadPosition() {
    readPosition_ = 0;
}

void SerializationBuffer::setReadPosition(size_t position) {
    readPosition_ = std::min(position, buffer_.size());
}

void SerializationBuffer::writeBytes(const std::byte* data, size_t size) {
    if (size == 0) return;

    size_t neededSize = buffer_.size() + size;
    if (buffer_.capacity() < neededSize) {
        buffer_.reserve(std::max(neededSize, buffer_.capacity() * 2));
    }

    buffer_.insert(buffer_.end(), data, data + size);
}

void SerializationBuffer::ensureSpace(size_t needed) {
    if (buffer_.capacity() < buffer_.size() + needed) {
        buffer_.reserve(std::max(buffer_.size() + needed, buffer_.capacity() * 2));
    }
}

// Serialization namespace implementations
namespace Serialization {

SerializationResult<bool> serialize(bool value) {
    return SerializationResult<bool>(value, true);
}

SerializationResult<bool> deserialize(const SerializationBuffer& buffer) {
    if (buffer.remaining() < sizeof(bool)) {
        return SerializationResult<bool>(false, "Insufficient data for bool");
    }
    return SerializationResult<bool>(buffer.readBool(), true);
}

SerializationResult<std::string> serialize(const std::string& value) {
    return SerializationResult<std::string>(value, true);
}

SerializationResult<std::string> deserialize(const SerializationBuffer& buffer) {
    try {
        std::string result = buffer.readString();
        return SerializationResult<std::string>(result, true);
    } catch (const std::exception& e) {
        return SerializationResult<std::string>("", std::string("Deserialization error: ") + e.what());
    }
}

SerializationResult<Vector3> serialize(const Vector3& value) {
    return SerializationResult<Vector3>(value, true);
}

SerializationResult<Vector3> deserialize(const SerializationBuffer& buffer) {
    try {
        Vector3 result = buffer.readVector3();
        return SerializationResult<Vector3>(result, true);
    } catch (const std::exception& e) {
        return SerializationResult<Vector3>(Vector3{}, std::string("Deserialization error: ") + e.what());
    }
}

SerializationResult<Quaternion> serialize(const Quaternion& value) {
    return SerializationResult<Quaternion>(value, true);
}

SerializationResult<Quaternion> deserialize(const SerializationBuffer& buffer) {
    try {
        Quaternion result = buffer.readQuaternion();
        return SerializationResult<Quaternion>(result, true);
    } catch (const std::exception& e) {
        return SerializationResult<Quaternion>(Quaternion{}, std::string("Deserialization error: ") + e.what());
    }
}

SerializationResult<Matrix4> serialize(const Matrix4& value) {
    return SerializationResult<Matrix4>(value, true);
}

SerializationResult<Matrix4> deserialize(const SerializationBuffer& buffer) {
    try {
        Matrix4 result = buffer.readMatrix4();
        return SerializationResult<Matrix4>(result, true);
    } catch (const std::exception& e) {
        return SerializationResult<Matrix4>(Matrix4{}, std::string("Deserialization error: ") + e.what());
    }
}

} // namespace Serialization

} // namespace FoundryEngine
