/**
 * @file AdvancedCharacterSystem.h
 * @brief Advanced character creation and animation system with Blender-level tools
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 *
 * This file contains the advanced character creation system for FoundryEngine.
 * Features include procedural character generation, advanced rigging tools,
 * facial animation, motion capture integration, and Blender-level character
 * customization capabilities.
 *
 * Key Features:
 * - AI-driven procedural character generation
 * - Advanced automatic rigging with IK/FK chains
 * - Facial animation with blend shapes and bone-based rigs
 * - Real-time motion capture integration
 * - Modular character customization system
 * - Animation state machines and blend trees
 * - Cloth and hair simulation integration
 * - Performance optimization for real-time rendering
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include "../math/Matrix4.h"
#include "../math/Quaternion.h"
#include "../graphics/Mesh.h"
#include "../animation/AnimationClip.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

namespace FoundryEngine {

// Forward declarations
class CharacterMesh;
class CharacterRig;
class FacialRig;
class AnimationController;
class MotionCaptureSystem;
class CharacterCustomizer;
class BlendShapeController;
class IKSolver;
class ClothSimulation;
class HairSimulation;

/**
 * @brief Character generation methods
 */
enum class CharacterGenerationMethod {
    Procedural,         ///< AI-driven procedural generation
    Template,           ///< Template-based generation
    Scan,              ///< 3D scan-based generation
    Hybrid             ///< Combination of methods
};

/**
 * @brief Character body types
 */
enum class BodyType {
    Humanoid,          ///< Standard humanoid character
    Creature,          ///< Non-humanoid creature
    Robot,             ///< Mechanical character
    Fantasy,           ///< Fantasy creature (elf, orc, etc.)
    Custom             ///< Custom body type
};

/**
 * @brief Animation quality levels
 */
enum class AnimationQuality {
    Low,               ///< Low quality for mobile/performance
    Medium,            ///< Medium quality for standard gameplay
    High,              ///< High quality for cutscenes
    Cinematic          ///< Cinematic quality for offline rendering
};

/**
 * @brief Character generation parameters
 */
struct CharacterGenerationParams {
    BodyType bodyType = BodyType::Humanoid;
    CharacterGenerationMethod method = CharacterGenerationMethod::Procedural;
    
    // Physical attributes
    float height = 1.75f;                      ///< Character height in meters
    float weight = 70.0f;                      ///< Character weight in kg
    float muscularity = 0.5f;                  ///< Muscle definition (0-1)
    float bodyFat = 0.2f;                      ///< Body fat percentage (0-1)
    
    // Proportions
    float headSize = 1.0f;                     ///< Head size multiplier
    float limbLength = 1.0f;                   ///< Limb length multiplier
    float torsoLength = 1.0f;                  ///< Torso length multiplier
    float shoulderWidth = 1.0f;                ///< Shoulder width multiplier
    float hipWidth = 1.0f;                     ///< Hip width multiplier
    
    // Facial features
    float eyeSize = 1.0f;                      ///< Eye size multiplier
    float noseSize = 1.0f;                     ///< Nose size multiplier
    float mouthSize = 1.0f;                    ///< Mouth size multiplier
    float earSize = 1.0f;                      ///< Ear size multiplier
    float jawWidth = 1.0f;                     ///< Jaw width multiplier
    float cheekboneHeight = 1.0f;              ///< Cheekbone prominence
    
    // Skin and appearance
    Vector3 skinColor = Vector3(0.8f, 0.7f, 0.6f);  ///< Base skin color
    float skinRoughness = 0.5f;                ///< Skin surface roughness
    float skinSubsurface = 0.3f;               ///< Subsurface scattering amount
    
    // Hair
    Vector3 hairColor = Vector3(0.3f, 0.2f, 0.1f);  ///< Hair color
    float hairLength = 0.1f;                   ///< Hair length
    float hairCurliness = 0.0f;                ///< Hair curl amount (0-1)
    float hairDensity = 1.0f;                  ///< Hair density
    
    // Eyes
    Vector3 eyeColor = Vector3(0.3f, 0.5f, 0.2f);   ///< Eye color
    float pupilSize = 0.5f;                    ///< Pupil size
    
    // Clothing and accessories
    std::vector<std::string> clothingItems;    ///< List of clothing items
    std::vector<std::string> accessories;      ///< List of accessories
    
    // Animation preferences
    AnimationQuality animationQuality = AnimationQuality::High;
    bool enableFacialAnimation = true;         ///< Enable facial animation
    bool enableClothSimulation = true;         ///< Enable cloth simulation
    bool enableHairSimulation = true;          ///< Enable hair physics
    bool enableMuscleDeformation = true;       ///< Enable muscle deformation
    
    // AI generation parameters
    std::string stylePrompt;                   ///< AI style prompt
    std::string personalityTraits;             ///< Character personality
    int randomSeed = -1;                       ///< Random seed (-1 for random)
};

/**
 * @brief Bone definition for character rigging
 */
struct BoneDefinition {
    std::string name;                          ///< Bone name
    std::string parentName;                    ///< Parent bone name
    Vector3 position;                          ///< Bone position
    Quaternion rotation;                       ///< Bone rotation
    Vector3 scale = Vector3(1.0f);             ///< Bone scale
    float length = 1.0f;                       ///< Bone length
    bool isIKTarget = false;                   ///< Is this an IK target bone
    bool isIKPole = false;                     ///< Is this an IK pole bone
    std::vector<std::string> constraints;      ///< Bone constraints
};

/**
 * @brief Blend shape definition
 */
struct BlendShape {
    std::string name;                          ///< Blend shape name
    std::vector<Vector3> deltaVertices;        ///< Vertex position deltas
    std::vector<Vector3> deltaNormals;         ///< Normal deltas
    float weight = 0.0f;                       ///< Current weight (0-1)
    float minWeight = 0.0f;                    ///< Minimum weight
    float maxWeight = 1.0f;                    ///< Maximum weight
    std::string category;                      ///< Blend shape category
};

/**
 * @brief IK chain definition
 */
struct IKChain {
    std::string name;                          ///< Chain name
    std::string rootBone;                      ///< Root bone of the chain
    std::string targetBone;                    ///< Target bone (end effector)
    std::string poleBone;                      ///< Pole target bone (optional)
    int chainLength = 2;                       ///< Number of bones in chain
    float weight = 1.0f;                       ///< IK weight (0-1)
    bool enabled = true;                       ///< Is chain enabled
    Vector3 targetPosition;                    ///< Target position
    Quaternion targetRotation;                 ///< Target rotation
    Vector3 polePosition;                      ///< Pole position
};

/**
 * @class AdvancedCharacterSystem
 * @brief Advanced character creation and animation system
 */
class AdvancedCharacterSystem : public System {
public:
    AdvancedCharacterSystem();
    ~AdvancedCharacterSystem();

    // System interface
    bool initialize() override;
    void shutdown() override;
    void update(float deltaTime) override;

    // Character generation
    std::shared_ptr<class Character> generateCharacter(const CharacterGenerationParams& params);
    std::shared_ptr<class Character> generateFromTemplate(const std::string& templateName, 
                                                         const CharacterGenerationParams& overrides = {});
    std::shared_ptr<class Character> generateFromScan(const std::string& scanDataPath,
                                                     const CharacterGenerationParams& params = {});
    
    // Character templates
    void registerCharacterTemplate(const std::string& name, const CharacterGenerationParams& params);
    void unregisterCharacterTemplate(const std::string& name);
    std::vector<std::string> getAvailableTemplates() const;
    CharacterGenerationParams getTemplate(const std::string& name) const;

    // Character management
    void addCharacter(std::shared_ptr<class Character> character);
    void removeCharacter(std::shared_ptr<class Character> character);
    std::vector<std::shared_ptr<class Character>> getAllCharacters() const;
    std::shared_ptr<class Character> findCharacter(const std::string& name) const;

    // Rigging tools
    std::shared_ptr<CharacterRig> createRig(std::shared_ptr<CharacterMesh> mesh, BodyType bodyType);
    std::shared_ptr<CharacterRig> createCustomRig(std::shared_ptr<CharacterMesh> mesh, 
                                                  const std::vector<BoneDefinition>& bones);
    void autoRig(std::shared_ptr<Character> character);
    
    // Facial animation
    std::shared_ptr<FacialRig> createFacialRig(std::shared_ptr<CharacterMesh> mesh);
    void generateFacialBlendShapes(std::shared_ptr<Character> character);
    void addBlendShape(std::shared_ptr<Character> character, const BlendShape& blendShape);

    // Motion capture integration
    MotionCaptureSystem* getMotionCaptureSystem() const { return motionCapture_.get(); }
    void enableMotionCapture(bool enable) { motionCaptureEnabled_ = enable; }
    bool isMotionCaptureEnabled() const { return motionCaptureEnabled_; }

    // Character customization
    CharacterCustomizer* getCharacterCustomizer() const { return customizer_.get(); }
    void customizeCharacter(std::shared_ptr<class Character> character, 
                           const std::unordered_map<std::string, float>& parameters);

    // Animation
    void playAnimation(std::shared_ptr<class Character> character, const std::string& animationName);
    void blendAnimations(std::shared_ptr<class Character> character, 
                        const std::vector<std::pair<std::string, float>>& animations);
    void setAnimationSpeed(std::shared_ptr<class Character> character, float speed);

    // IK solving
    void addIKChain(std::shared_ptr<class Character> character, const IKChain& chain);
    void removeIKChain(std::shared_ptr<class Character> character, const std::string& chainName);
    void updateIK(std::shared_ptr<class Character> character);

    // Cloth and hair simulation
    void enableClothSimulation(std::shared_ptr<class Character> character, bool enable);
    void enableHairSimulation(std::shared_ptr<class Character> character, bool enable);
    void addClothPiece(std::shared_ptr<class Character> character, const std::string& clothingItem);
    void removeClothPiece(std::shared_ptr<class Character> character, const std::string& clothingItem);

    // Performance optimization
    void setLODLevel(std::shared_ptr<class Character> character, int lodLevel);
    void enableGPUSkinnning(bool enable) { gpuSkinning_ = enable; }
    bool isGPUSkinningEnabled() const { return gpuSkinning_; }

    // Serialization
    std::string serializeCharacter(std::shared_ptr<class Character> character) const;
    std::shared_ptr<class Character> deserializeCharacter(const std::string& data);
    void saveCharacterToFile(std::shared_ptr<class Character> character, const std::string& filename);
    std::shared_ptr<class Character> loadCharacterFromFile(const std::string& filename);

private:
    class AdvancedCharacterSystemImpl;
    std::unique_ptr<AdvancedCharacterSystemImpl> impl_;

    std::vector<std::shared_ptr<class Character>> characters_;
    std::unordered_map<std::string, CharacterGenerationParams> templates_;
    
    std::unique_ptr<MotionCaptureSystem> motionCapture_;
    std::unique_ptr<CharacterCustomizer> customizer_;
    
    bool motionCaptureEnabled_ = false;
    bool gpuSkinning_ = true;
    
    // Internal methods
    void initializeTemplates();
    void setupDefaultRigs();
    std::shared_ptr<CharacterMesh> generateMesh(const CharacterGenerationParams& params);
    void applyProceduralGeneration(std::shared_ptr<CharacterMesh> mesh, const CharacterGenerationParams& params);
    void generateSkinTextures(std::shared_ptr<class Character> character, const CharacterGenerationParams& params);
    void generateHair(std::shared_ptr<class Character> character, const CharacterGenerationParams& params);
    void optimizeForPerformance(std::shared_ptr<class Character> character);
};

/**
 * @class Character
 * @brief Complete character representation with mesh, rig, and animation
 */
class Character {
public:
    Character(const std::string& name);
    ~Character();

    // Basic properties
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    // Mesh and geometry
    std::shared_ptr<CharacterMesh> getMesh() const { return mesh_; }
    void setMesh(std::shared_ptr<CharacterMesh> mesh) { mesh_ = mesh; }

    // Rigging
    std::shared_ptr<CharacterRig> getRig() const { return rig_; }
    void setRig(std::shared_ptr<CharacterRig> rig) { rig_ = rig; }
    
    std::shared_ptr<FacialRig> getFacialRig() const { return facialRig_; }
    void setFacialRig(std::shared_ptr<FacialRig> facialRig) { facialRig_ = facialRig; }

    // Animation
    std::shared_ptr<AnimationController> getAnimationController() const { return animationController_; }
    void setAnimationController(std::shared_ptr<AnimationController> controller) { animationController_ = controller; }

    // Blend shapes
    void addBlendShape(const BlendShape& blendShape);
    void removeBlendShape(const std::string& name);
    BlendShape* getBlendShape(const std::string& name);
    const std::vector<BlendShape>& getAllBlendShapes() const { return blendShapes_; }
    void setBlendShapeWeight(const std::string& name, float weight);
    float getBlendShapeWeight(const std::string& name) const;

    // IK chains
    void addIKChain(const IKChain& chain);
    void removeIKChain(const std::string& name);
    IKChain* getIKChain(const std::string& name);
    const std::vector<IKChain>& getAllIKChains() const { return ikChains_; }

    // Cloth and hair
    std::shared_ptr<ClothSimulation> getClothSimulation() const { return clothSimulation_; }
    void setClothSimulation(std::shared_ptr<ClothSimulation> cloth) { clothSimulation_ = cloth; }
    
    std::shared_ptr<HairSimulation> getHairSimulation() const { return hairSimulation_; }
    void setHairSimulation(std::shared_ptr<HairSimulation> hair) { hairSimulation_ = hair; }

    // Transform
    void setPosition(const Vector3& position) { position_ = position; }
    const Vector3& getPosition() const { return position_; }
    
    void setRotation(const Quaternion& rotation) { rotation_ = rotation; }
    const Quaternion& getRotation() const { return rotation_; }
    
    void setScale(const Vector3& scale) { scale_ = scale; }
    const Vector3& getScale() const { return scale_; }

    Matrix4 getTransformMatrix() const;

    // LOD and performance
    void setLODLevel(int level) { lodLevel_ = level; }
    int getLODLevel() const { return lodLevel_; }
    
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    // Update
    void update(float deltaTime);
    void render(const Matrix4& viewMatrix, const Matrix4& projMatrix);

    // Serialization
    std::string serialize() const;
    bool deserialize(const std::string& data);

private:
    std::string name_;
    
    std::shared_ptr<CharacterMesh> mesh_;
    std::shared_ptr<CharacterRig> rig_;
    std::shared_ptr<FacialRig> facialRig_;
    std::shared_ptr<AnimationController> animationController_;
    std::shared_ptr<ClothSimulation> clothSimulation_;
    std::shared_ptr<HairSimulation> hairSimulation_;
    
    std::vector<BlendShape> blendShapes_;
    std::vector<IKChain> ikChains_;
    
    Vector3 position_ = Vector3::zero();
    Quaternion rotation_ = Quaternion::identity();
    Vector3 scale_ = Vector3(1.0f);
    
    int lodLevel_ = 0;
    bool visible_ = true;
    
    void updateBlendShapes();
    void updateIK();
    void updateCloth(float deltaTime);
    void updateHair(float deltaTime);
};

/**
 * @class CharacterRig
 * @brief Character skeleton and rigging system
 */
class CharacterRig {
public:
    CharacterRig();
    ~CharacterRig();

    // Bone management
    void addBone(const BoneDefinition& bone);
    void removeBone(const std::string& name);
    class Bone* getBone(const std::string& name);
    const std::vector<std::unique_ptr<class Bone>>& getAllBones() const { return bones_; }

    // Hierarchy
    void setBoneParent(const std::string& boneName, const std::string& parentName);
    void removeBoneParent(const std::string& boneName);
    std::vector<class Bone*> getChildBones(const std::string& parentName) const;
    class Bone* getRootBone() const;

    // Pose and animation
    void setPose(const std::unordered_map<std::string, Matrix4>& boneTransforms);
    void resetToBindPose();
    void applyAnimation(const class AnimationClip& clip, float time);
    
    // Skinning
    void bindToMesh(std::shared_ptr<CharacterMesh> mesh);
    void updateSkinning();
    const std::vector<Matrix4>& getBoneMatrices() const { return boneMatrices_; }

    // Constraints
    void addConstraint(const std::string& boneName, std::shared_ptr<class BoneConstraint> constraint);
    void removeConstraint(const std::string& boneName, const std::string& constraintName);
    void updateConstraints();

private:
    std::vector<std::unique_ptr<class Bone>> bones_;
    std::unordered_map<std::string, size_t> boneNameToIndex_;
    std::vector<Matrix4> boneMatrices_;
    std::vector<Matrix4> bindPoseMatrices_;
    
    void calculateBoneMatrices();
    void updateBoneHierarchy();
};

/**
 * @class FacialRig
 * @brief Facial animation and expression system
 */
class FacialRig {
public:
    /**
     * @brief Facial expression categories
     */
    enum class ExpressionCategory {
        Emotion,        ///< Emotional expressions
        Phoneme,        ///< Speech phonemes
        Corrective,     ///< Corrective shapes
        Custom          ///< Custom expressions
    };

    FacialRig();
    ~FacialRig();

    // Blend shapes
    void addBlendShape(const BlendShape& blendShape, ExpressionCategory category);
    void removeBlendShape(const std::string& name);
    BlendShape* getBlendShape(const std::string& name);
    std::vector<BlendShape*> getBlendShapesByCategory(ExpressionCategory category);

    // Expressions
    void setExpression(const std::string& expressionName, float intensity);
    void blendExpressions(const std::unordered_map<std::string, float>& expressions);
    void playExpressionSequence(const std::vector<std::pair<std::string, float>>& sequence, float duration);

    // Speech animation
    void speakText(const std::string& text, float duration);
    void setPhoneme(const std::string& phoneme, float intensity);
    void enableLipSync(bool enable) { lipSyncEnabled_ = enable; }

    // Eye control
    void setEyeTarget(const Vector3& target);
    void setEyeRotation(const Vector2& rotation); // pitch, yaw
    void enableEyeTracking(bool enable) { eyeTrackingEnabled_ = enable; }
    void blink(float duration = 0.2f);

    // Update
    void update(float deltaTime);
    void applyToMesh(std::shared_ptr<CharacterMesh> mesh);

private:
    std::vector<BlendShape> blendShapes_;
    std::unordered_map<ExpressionCategory, std::vector<size_t>> categoryMap_;
    
    bool lipSyncEnabled_ = false;
    bool eyeTrackingEnabled_ = false;
    Vector3 eyeTarget_;
    Vector2 eyeRotation_;
    
    struct ExpressionKeyframe {
        std::string expression;
        float intensity;
        float time;
    };
    std::vector<ExpressionKeyframe> currentSequence_;
    float sequenceTime_ = 0.0f;
    
    void updateExpressionSequence(float deltaTime);
    void updateEyeMovement(float deltaTime);
    void updateLipSync(float deltaTime);
    std::vector<std::string> textToPhonemes(const std::string& text);
};

/**
 * @class MotionCaptureSystem
 * @brief Real-time motion capture integration
 */
class MotionCaptureSystem {
public:
    /**
     * @brief Motion capture device types
     */
    enum class DeviceType {
        OpticalMarkers,     ///< Optical marker-based system
        Inertial,          ///< Inertial measurement units
        Computer Vision,    ///< Computer vision-based
        Hybrid             ///< Combination of systems
    };

    MotionCaptureSystem();
    ~MotionCaptureSystem();

    bool initialize(DeviceType deviceType);
    void shutdown();

    // Device management
    bool connectDevice(const std::string& deviceName);
    void disconnectDevice(const std::string& deviceName);
    std::vector<std::string> getAvailableDevices() const;
    bool isDeviceConnected(const std::string& deviceName) const;

    // Calibration
    void startCalibration();
    void stopCalibration();
    bool isCalibrated() const { return calibrated_; }
    void saveCalibration(const std::string& filename);
    void loadCalibration(const std::string& filename);

    // Recording
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording_; }
    void saveRecording(const std::string& filename);
    std::shared_ptr<class AnimationClip> getRecordedAnimation();

    // Real-time streaming
    void startStreaming(std::shared_ptr<Character> character);
    void stopStreaming();
    bool isStreaming() const { return streaming_; }

    // Data processing
    void setSmoothing(float amount) { smoothing_ = amount; }
    void setNoiseReduction(float amount) { noiseReduction_ = amount; }
    void enablePrediction(bool enable) { predictionEnabled_ = enable; }

private:
    DeviceType deviceType_;
    bool calibrated_ = false;
    bool recording_ = false;
    bool streaming_ = false;
    float smoothing_ = 0.1f;
    float noiseReduction_ = 0.05f;
    bool predictionEnabled_ = true;
    
    std::vector<std::string> connectedDevices_;
    std::shared_ptr<Character> streamingTarget_;
    std::vector<class MotionFrame> recordedFrames_;
    
    void processMotionData(const class MotionFrame& frame);
    void applyToCharacter(std::shared_ptr<Character> character, const class MotionFrame& frame);
    class MotionFrame smoothFrame(const class MotionFrame& frame);
    class MotionFrame reduceNoise(const class MotionFrame& frame);
};

} // namespace FoundryEngine