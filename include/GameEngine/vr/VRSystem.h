/**
 * @file VRSystem.h
 * @brief Comprehensive VR/AR system supporting all major platforms
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include "../math/Quaternion.h"
#include "../math/Matrix4.h"
#include <memory>
#include <vector>
#include <functional>

namespace FoundryEngine {

/**
 * @class VRSystem
 * @brief Universal VR/AR system supporting Quest, PICO, HoloLens, Vision Pro
 */
class VRSystem : public System {
public:
    enum class VRPlatform {
        None,
        OculusQuest,
        PICO,
        HoloLens,
        AppleVisionPro,
        SteamVR,
        OpenXR,
        WebXR
    };

    enum class TrackingSpace {
        Seated,
        Standing,
        RoomScale,
        WorldScale
    };

    enum class HandTracking {
        None,
        Controllers,
        Hands,
        Mixed
    };

    struct VRConfig {
        VRPlatform platform = VRPlatform::OpenXR;
        TrackingSpace trackingSpace = TrackingSpace::RoomScale;
        HandTracking handTracking = HandTracking::Mixed;
        bool enablePassthrough = false;
        bool enableEyeTracking = false;
        bool enableFaceTracking = false;
        bool enableSpatialAnchors = true;
        float renderScale = 1.0f;
        uint32_t refreshRate = 90;
    };

    struct HMDInfo {
        std::string deviceName;
        Vector3 position;
        Quaternion rotation;
        Matrix4 leftEyeProjection;
        Matrix4 rightEyeProjection;
        Matrix4 leftEyeView;
        Matrix4 rightEyeView;
        float ipd; // Interpupillary distance
        bool isConnected;
    };

    struct ControllerInfo {
        uint32_t controllerId;
        Vector3 position;
        Quaternion rotation;
        Vector3 velocity;
        Vector3 angularVelocity;
        bool isConnected;
        std::vector<bool> buttonStates;
        std::vector<float> axisValues;
        float batteryLevel;
        bool isTracked;
    };

    struct HandInfo {
        Vector3 wristPosition;
        Quaternion wristRotation;
        std::vector<Vector3> fingerPositions; // 25 joints per hand
        std::vector<Quaternion> fingerRotations;
        float confidence;
        bool isTracked;
    };

    VRSystem();
    ~VRSystem();

    bool initialize(const VRConfig& config = VRConfig{}) override;
    void shutdown() override;
    void update(float deltaTime) override;

    // Platform detection and initialization
    std::vector<VRPlatform> getAvailablePlatforms() const;
    bool isPlatformSupported(VRPlatform platform) const;
    VRPlatform getCurrentPlatform() const;
    bool switchPlatform(VRPlatform platform);

    // HMD tracking
    HMDInfo getHMDInfo() const;
    Matrix4 getHMDPose() const;
    void recenterTracking();
    void setTrackingSpace(TrackingSpace space);
    TrackingSpace getTrackingSpace() const;

    // Controller input
    std::vector<ControllerInfo> getControllers() const;
    ControllerInfo getController(uint32_t controllerId) const;
    bool isControllerConnected(uint32_t controllerId) const;
    void setControllerVibration(uint32_t controllerId, float intensity, float duration);

    // Hand tracking
    HandInfo getLeftHand() const;
    HandInfo getRightHand() const;
    bool isHandTrackingEnabled() const;
    void enableHandTracking(bool enable);

    // Eye tracking
    Vector3 getEyeGazeDirection() const;
    Vector3 getEyeGazeOrigin() const;
    float getEyeOpenness(bool leftEye) const;
    bool isEyeTrackingEnabled() const;
    void enableEyeTracking(bool enable);

    // Spatial anchors
    uint32_t createSpatialAnchor(const Vector3& position, const Quaternion& rotation);
    void destroySpatialAnchor(uint32_t anchorId);
    Matrix4 getSpatialAnchorPose(uint32_t anchorId) const;
    bool isSpatialAnchorTracked(uint32_t anchorId) const;
    void saveSpatialAnchor(uint32_t anchorId, const std::string& name);
    uint32_t loadSpatialAnchor(const std::string& name);

    // Passthrough (AR)
    void enablePassthrough(bool enable);
    bool isPassthroughEnabled() const;
    void setPassthroughOpacity(float opacity);
    float getPassthroughOpacity() const;

    // Rendering
    void beginFrame();
    void endFrame();
    void submitFrame(uint32_t eyeIndex, uint32_t textureId);
    Vector2 getRenderTargetSize() const;
    void setRenderScale(float scale);
    float getRenderScale() const;

    // Haptic feedback
    void triggerHapticPulse(uint32_t controllerId, float intensity, float duration);
    void playHapticPattern(uint32_t controllerId, const std::vector<float>& pattern);
    void stopHapticFeedback(uint32_t controllerId);

    // Performance monitoring
    float getFrameRate() const;
    float getFrameTime() const;
    uint32_t getDroppedFrames() const;
    float getGPUUtilization() const;

    // Event callbacks
    void setControllerConnectedCallback(std::function<void(uint32_t)> callback);
    void setControllerDisconnectedCallback(std::function<void(uint32_t)> callback);
    void setTrackingLostCallback(std::function<void()> callback);
    void setTrackingRecoveredCallback(std::function<void()> callback);

private:
    class VRSystemImpl;
    std::unique_ptr<VRSystemImpl> impl_;
};

/**
 * @class SpatialMapping
 * @brief Real-time spatial mapping and mesh generation for AR
 */
class SpatialMapping {
public:
    struct MeshData {
        std::vector<Vector3> vertices;
        std::vector<uint32_t> indices;
        std::vector<Vector3> normals;
        std::vector<Vector2> uvs;
        float confidence;
        std::chrono::steady_clock::time_point timestamp;
    };

    struct SpatialMesh {
        uint32_t meshId;
        MeshData data;
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;
        bool isStatic;
    };

    SpatialMapping();
    ~SpatialMapping();

    void initialize();
    void shutdown();
    void update(float deltaTime);

    // Mesh management
    void startMapping();
    void stopMapping();
    bool isMappingActive() const;
    void clearMeshes();
    
    std::vector<SpatialMesh> getAllMeshes() const;
    SpatialMesh getMesh(uint32_t meshId) const;
    void removeMesh(uint32_t meshId);

    // Configuration
    void setMeshDensity(float density);
    void setMappingRange(float range);
    void setConfidenceThreshold(float threshold);
    void enableOcclusion(bool enable);

    // Collision detection
    bool raycast(const Vector3& origin, const Vector3& direction, float maxDistance,
                Vector3& hitPoint, Vector3& hitNormal, uint32_t& meshId) const;
    std::vector<uint32_t> getMeshesInBounds(const Vector3& min, const Vector3& max) const;

    // Callbacks
    void setMeshAddedCallback(std::function<void(uint32_t)> callback);
    void setMeshUpdatedCallback(std::function<void(uint32_t)> callback);
    void setMeshRemovedCallback(std::function<void(uint32_t)> callback);

private:
    class SpatialMappingImpl;
    std::unique_ptr<SpatialMappingImpl> impl_;
};

/**
 * @class VRInteraction
 * @brief Advanced VR interaction system with gesture recognition
 */
class VRInteraction {
public:
    enum class InteractionType {
        Point,
        Grab,
        Pinch,
        Poke,
        Gesture,
        Voice
    };

    struct InteractionEvent {
        InteractionType type;
        uint32_t objectId;
        Vector3 position;
        Vector3 direction;
        float strength;
        std::string gestureType;
    };

    VRInteraction();
    ~VRInteraction();

    void initialize();
    void shutdown();
    void update(float deltaTime);

    // Object registration
    void registerInteractable(uint32_t objectId, const std::vector<InteractionType>& types);
    void unregisterInteractable(uint32_t objectId);
    bool isInteractable(uint32_t objectId) const;

    // Gesture recognition
    void registerGesture(const std::string& name, const std::vector<Vector3>& pattern);
    void unregisterGesture(const std::string& name);
    std::string recognizeGesture(const std::vector<Vector3>& handPath) const;

    // Voice commands
    void registerVoiceCommand(const std::string& command, std::function<void()> callback);
    void unregisterVoiceCommand(const std::string& command);
    void enableVoiceRecognition(bool enable);

    // Interaction detection
    std::vector<InteractionEvent> getInteractionEvents() const;
    void setInteractionCallback(std::function<void(const InteractionEvent&)> callback);

    // Physics interaction
    void enablePhysicsInteraction(bool enable);
    void setGrabStrength(float strength);
    void setThrowVelocityMultiplier(float multiplier);

private:
    class VRInteractionImpl;
    std::unique_ptr<VRInteractionImpl> impl_;
};

} // namespace FoundryEngine