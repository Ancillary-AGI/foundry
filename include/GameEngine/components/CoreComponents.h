#pragma once

#include "../core/Component.h"
#include "../math/Vector3.h"
#include "../math/Quaternion.h"
#include "../math/Matrix4.h"
#include <string>
#include <vector>
#include <memory>

namespace FoundryEngine {

class Mesh;
class Material;
class Texture;
class AudioClip;
class AudioSource;
class RigidBody;
class Collider;
class Light;
class Camera;
class Script;

// Transform Component
class TransformComponent : public Component {
public:
    Vector3 position{0, 0, 0};
    Quaternion rotation{0, 0, 0, 1};
    Vector3 scale{1, 1, 1};
    
    TransformComponent* parent = nullptr;
    std::vector<TransformComponent*> children;
    
    Matrix4 getLocalMatrix() const;
    Matrix4 getWorldMatrix() const;
    Vector3 getWorldPosition() const;
    Quaternion getWorldRotation() const;
    Vector3 getWorldScale() const;
    
    Vector3 getForward() const;
    Vector3 getRight() const;
    Vector3 getUp() const;
    
    void setParent(TransformComponent* newParent);
    void addChild(TransformComponent* child);
    void removeChild(TransformComponent* child);
    
    void translate(const Vector3& translation);
    void rotate(const Quaternion& rotation);
    void rotateAround(const Vector3& point, const Vector3& axis, float angle);
    void lookAt(const Vector3& target, const Vector3& up = Vector3(0, 1, 0));
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

// Mesh Renderer Component
class MeshRendererComponent : public Component {
public:
    Mesh* mesh = nullptr;
    std::vector<Material*> materials;
    bool castShadows = true;
    bool receiveShadows = true;
    int renderLayer = 0;
    float lodBias = 1.0f;
    
    void setMesh(Mesh* newMesh);
    Mesh* getMesh() const { return mesh; }
    
    void setMaterial(Material* material, int index = 0);
    Material* getMaterial(int index = 0) const;
    void addMaterial(Material* material);
    void removeMaterial(int index);
    int getMaterialCount() const { return static_cast<int>(materials.size()); }
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

// Camera Component
class CameraComponent : public Component {
public:
    enum class ProjectionType {
        Perspective,
        Orthographic
    };
    
    ProjectionType projectionType = ProjectionType::Perspective;
    float fieldOfView = 60.0f;
    float orthographicSize = 5.0f;
    float nearClip = 0.1f;
    float farClip = 1000.0f;
    float aspect = 16.0f / 9.0f;
    
    int renderLayer = -1; // -1 means render all layers
    int cullingMask = 0xFFFFFFFF;
    
    Matrix4 getProjectionMatrix() const;
    Matrix4 getViewMatrix() const;
    Matrix4 getViewProjectionMatrix() const;
    
    Vector3 screenToWorldPoint(const Vector3& screenPoint) const;
    Vector3 worldToScreenPoint(const Vector3& worldPoint) const;
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

// Light Component
class LightComponent : public Component {
public:
    enum class LightType {
        Directional,
        Point,
        Spot,
        Area
    };
    
    LightType type = LightType::Directional;
    Vector3 color{1, 1, 1};
    float intensity = 1.0f;
    float range = 10.0f;
    float spotAngle = 30.0f;
    float innerSpotAngle = 20.0f;
    
    bool castShadows = true;
    int shadowMapSize = 1024;
    float shadowBias = 0.001f;
    float shadowNormalBias = 0.1f;
    float shadowNearPlane = 0.1f;
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

// Audio Source Component
class AudioSourceComponent : public Component {
public:
    AudioClip* clip = nullptr;
    float volume = 1.0f;
    float pitch = 1.0f;
    bool loop = false;
    bool playOnAwake = false;
    bool is3D = true;
    
    float minDistance = 1.0f;
    float maxDistance = 500.0f;
    int rolloffMode = 0; // 0 = Logarithmic, 1 = Linear, 2 = Custom
    float spatialBlend = 1.0f; // 0 = 2D, 1 = 3D
    
    AudioSource* audioSource = nullptr;
    
    void play();
    void pause();
    void stop();
    bool isPlaying() const;
    
    void setClip(AudioClip* newClip);
    AudioClip* getClip() const { return clip; }
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

// Rigidbody Component
class RigidbodyComponent : public Component {
public:
    enum class BodyType {
        Dynamic,
        Kinematic,
        Static
    };
    
    BodyType bodyType = BodyType::Dynamic;
    float mass = 1.0f;
    float drag = 0.0f;
    float angularDrag = 0.05f;
    bool useGravity = true;
    bool isKinematic = false;
    bool freezeRotation = false;
    
    Vector3 velocity{0, 0, 0};
    Vector3 angularVelocity{0, 0, 0};
    Vector3 centerOfMass{0, 0, 0};
    
    RigidBody* rigidBody = nullptr;
    
    void addForce(const Vector3& force);
    void addForceAtPosition(const Vector3& force, const Vector3& position);
    void addTorque(const Vector3& torque);
    void addExplosionForce(float explosionForce, const Vector3& explosionPosition, float explosionRadius);
    
    void setVelocity(const Vector3& newVelocity);
    Vector3 getVelocity() const;
    
    void setAngularVelocity(const Vector3& newAngularVelocity);
    Vector3 getAngularVelocity() const;
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

// Collider Component (Base)
class ColliderComponent : public Component {
public:
    enum class ColliderType {
        Box,
        Sphere,
        Capsule,
        Mesh,
        Terrain
    };
    
    ColliderType type;
    bool isTrigger = false;
    Vector3 center{0, 0, 0};
    int layer = 0;
    
    Collider* collider = nullptr;
    
    virtual void updateCollider() = 0;
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
};

// Box Collider Component
class BoxColliderComponent : public ColliderComponent {
public:
    Vector3 size{1, 1, 1};
    
    BoxColliderComponent() { type = ColliderType::Box; }
    
    void updateCollider() override;
    Component* clone() const override;
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
};

// Sphere Collider Component
class SphereColliderComponent : public ColliderComponent {
public:
    float radius = 0.5f;
    
    SphereColliderComponent() { type = ColliderType::Sphere; }
    
    void updateCollider() override;
    Component* clone() const override;
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
};

// Capsule Collider Component
class CapsuleColliderComponent : public ColliderComponent {
public:
    float radius = 0.5f;
    float height = 2.0f;
    int direction = 1; // 0 = X, 1 = Y, 2 = Z
    
    CapsuleColliderComponent() { type = ColliderType::Capsule; }
    
    void updateCollider() override;
    Component* clone() const override;
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
};

// Mesh Collider Component
class MeshColliderComponent : public ColliderComponent {
public:
    Mesh* mesh = nullptr;
    bool convex = false;
    
    MeshColliderComponent() { type = ColliderType::Mesh; }
    
    void setMesh(Mesh* newMesh);
    Mesh* getMesh() const { return mesh; }
    
    void updateCollider() override;
    Component* clone() const override;
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
};

// Script Component
class ScriptComponent : public Component {
public:
    std::string scriptPath;
    Script* script = nullptr;
    
    void setScript(const std::string& path);
    Script* getScript() const { return script; }
    
    void callFunction(const std::string& functionName);
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

// Animator Component
class AnimatorComponent : public Component {
public:
    std::string animatorControllerPath;
    std::unordered_map<std::string, float> floatParameters;
    std::unordered_map<std::string, int> intParameters;
    std::unordered_map<std::string, bool> boolParameters;
    std::unordered_map<std::string, std::string> triggerParameters;
    
    void setFloat(const std::string& name, float value);
    float getFloat(const std::string& name) const;
    
    void setInt(const std::string& name, int value);
    int getInt(const std::string& name) const;
    
    void setBool(const std::string& name, bool value);
    bool getBool(const std::string& name) const;
    
    void setTrigger(const std::string& name);
    void resetTrigger(const std::string& name);
    
    void play(const std::string& stateName, int layer = 0);
    void crossFade(const std::string& stateName, float transitionDuration, int layer = 0);
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

// Particle System Component
class ParticleSystemComponent : public Component {
public:
    struct MainModule {
        float duration = 5.0f;
        bool looping = true;
        bool prewarm = false;
        float startLifetime = 5.0f;
        float startSpeed = 5.0f;
        Vector3 startSize{1, 1, 1};
        Vector3 startRotation{0, 0, 0};
        Vector3 startColor{1, 1, 1};
        float gravityModifier = 0.0f;
        int maxParticles = 1000;
    } main;
    
    struct EmissionModule {
        bool enabled = true;
        float rateOverTime = 10.0f;
        float rateOverDistance = 0.0f;
    } emission;
    
    struct ShapeModule {
        bool enabled = true;
        int shapeType = 0; // 0 = Sphere, 1 = Box, 2 = Circle, etc.
        float radius = 1.0f;
        Vector3 box{1, 1, 1};
        float angle = 25.0f;
    } shape;
    
    struct VelocityOverLifetimeModule {
        bool enabled = false;
        Vector3 linear{0, 0, 0};
        Vector3 orbital{0, 0, 0};
        Vector3 offset{0, 0, 0};
        Vector3 radial{0, 0, 0};
        float speedModifier = 1.0f;
    } velocityOverLifetime;
    
    struct ColorOverLifetimeModule {
        bool enabled = false;
        // Color gradient would be implemented here
    } colorOverLifetime;
    
    struct SizeOverLifetimeModule {
        bool enabled = false;
        // Size curve would be implemented here
    } sizeOverLifetime;
    
    void play();
    void pause();
    void stop();
    void clear();
    bool isPlaying() const;
    bool isPaused() const;
    
    void emit(int count);
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

// Terrain Component
class TerrainComponent : public Component {
public:
    int heightmapWidth = 513;
    int heightmapHeight = 513;
    Vector3 terrainSize{1000, 100, 1000};
    std::vector<float> heightData;
    std::vector<Texture*> textures;
    std::vector<float> textureScales;
    
    void setHeightmapResolution(int width, int height);
    void setHeight(int x, int y, float height);
    float getHeight(int x, int y) const;
    float getInterpolatedHeight(float x, float z) const;
    
    void addTexture(Texture* texture, float scale = 1.0f);
    void removeTexture(int index);
    
    void generateHeightmap();
    void applyHeightmap();
    
    void serialize(void* data) override;
    void deserialize(const void* data) override;
    Component* clone() const override;
};

} // namespace FoundryEngine