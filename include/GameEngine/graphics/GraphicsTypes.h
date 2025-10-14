#pragma once

#include "../math/Vector3.h"
#include "../math/Matrix4.h"
#include <vector>
#include <string>

namespace FoundryEngine {

// Forward declarations
class Mesh;
class Material;
class Texture;
class Shader;
class Camera;
class Light;
class RenderTarget;

// Basic graphics types
struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector3 texCoord;
    Vector3 color;
};

struct Triangle {
    int indices[3];
};

// Mesh class
class Mesh {
public:
    virtual ~Mesh() = default;
    virtual bool load(const std::string& path) = 0;
    virtual void unload() = 0;
    virtual const std::vector<Vertex>& getVertices() const = 0;
    virtual const std::vector<Triangle>& getTriangles() const = 0;
    virtual bool isLoaded() const = 0;
};

// Material class
class Material {
public:
    virtual ~Material() = default;
    virtual void setShader(Shader* shader) = 0;
    virtual Shader* getShader() const = 0;
    virtual void setTexture(const std::string& name, Texture* texture) = 0;
    virtual Texture* getTexture(const std::string& name) const = 0;
    virtual void setProperty(const std::string& name, float value) = 0;
    virtual float getProperty(const std::string& name) const = 0;
};

// Texture class
class Texture {
public:
    virtual ~Texture() = default;
    virtual bool load(const std::string& path) = 0;
    virtual void unload() = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual bool isLoaded() const = 0;
};

// Shader class
class Shader {
public:
    virtual ~Shader() = default;
    virtual bool load(const std::string& vertexPath, const std::string& fragmentPath) = 0;
    virtual void unload() = 0;
    virtual void use() = 0;
    virtual void setUniform(const std::string& name, float value) = 0;
    virtual void setUniform(const std::string& name, const Vector3& value) = 0;
    virtual void setUniform(const std::string& name, const Matrix4& value) = 0;
    virtual bool isLoaded() const = 0;
};

// Camera class
class Camera {
public:
    virtual ~Camera() = default;
    virtual void setPosition(const Vector3& position) = 0;
    virtual Vector3 getPosition() const = 0;
    virtual void setRotation(const Vector3& rotation) = 0;
    virtual Vector3 getRotation() const = 0;
    virtual void setFOV(float fov) = 0;
    virtual float getFOV() const = 0;
    virtual void setNearPlane(float nearPlane) = 0;
    virtual float getNearPlane() const = 0;
    virtual void setFarPlane(float farPlane) = 0;
    virtual float getFarPlane() const = 0;
    virtual Matrix4 getViewMatrix() const = 0;
    virtual Matrix4 getProjectionMatrix() const = 0;
};

// Light class
class Light {
public:
    virtual ~Light() = default;
    virtual void setPosition(const Vector3& position) = 0;
    virtual Vector3 getPosition() const = 0;
    virtual void setColor(const Vector3& color) = 0;
    virtual Vector3 getColor() const = 0;
    virtual void setIntensity(float intensity) = 0;
    virtual float getIntensity() const = 0;
    virtual void setType(int type) = 0;
    virtual int getType() const = 0;
};

// RenderTarget class
class RenderTarget {
public:
    virtual ~RenderTarget() = default;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual bool isHDR() const = 0;
    virtual void bind() = 0;
    virtual void unbind() = 0;
};

} // namespace FoundryEngine
