#ifndef NEUTRAL_GAMEENGINE_ADVANCED_ANIMATION_H
#define NEUTRAL_GAMEENGINE_ADVANCED_ANIMATION_H

#include <vector>
#include <unordered_map>
#include <memory>
#include "../../math/Vector3.h"
#include "../../math/Quaternion.h"
#include "../../math/Matrix4.h"
#include "../System.h"

namespace NeutralGameEngine {

// Dual Quaternion Skinning for better deformations
class DualQuaternionSkinning {
public:
    struct BoneTransform {
        Quaternion rotation;
        Vector3 translation;
        float scale;

        // Dual quaternion representation
        Quaternion real;     // Rotation quaternion
        Quaternion dual;     // Rotation * (0.5 * translation) in dual space

        void updateDualQuaternion();
    };

    struct SkinningVertex {
        Vector3 position;
        Vector3 normal;
        std::vector<std::pair<int, float>> boneWeights; // Bone index -> weight
    };

    std::vector<BoneTransform> boneTransforms;
    std::vector<SkinningVertex> vertices;

    // Linear blend skinning (LBS)
    void linearBlendSkinning(std::vector<Vector3>& deformedPositions,
                           std::vector<Vector3>& deformedNormals);

    // Dual quaternion skinning (better for rotations)
    void dualQuaternionSkinning(std::vector<Vector3>& deformedPositions,
                              std::vector<Vector3>& deformedNormals);

    // Handle mesh with multiple influences
    void multiBoneSkinning(int maxInfluences = 4);
};

// Advanced Animation Blending
class AnimationBlender {
public:
    struct AnimationClip {
        std::string name;
        std::vector<std::vector<BoneTransform>> keyframes; // Frames[boneIndex]
        float duration;
        float fps;
        bool loop;
    };

    struct BlendNode {
        AnimationClip* clip;
        float time;
        float weight;
        float speed;
        bool active;

        // Blend parameters
        float blendTime;
        std::unordered_map<std::string, float> parameters; // User-defined parameters
    };

    std::vector<BlendNode> blendNodes;
    std::vector<BoneTransform> outputTransforms;

    // Motion matching for seamless blending
    struct MotionKeyframe {
        std::vector<BoneTransform> pose;
        Vector3 trajectoryDirection; // Future movement prediction
        float speed;

        // Feature vector for matching
        std::vector<float> features; // Velocity, trajectory, pose features
    };

    struct MotionDatabase {
        std::vector<MotionKeyframe> keyframes;
        // KD-tree for fast nearest neighbor search
        struct KDNode {
            int dimension;
            float value;
            KDNode* left;
            KDNode* right;
            int keyframeIndex;
        };
        KDNode* root;

        void buildKDTree();
        int findBestMatch(const std::vector<float>& queryFeatures) const;
    } motionDatabase;

    // Motion matching implementation
    AnimationClip* findMatchingMotion(const Vector3& desiredVelocity,
                                    const Vector3& desiredTrajectory,
                                    const std::vector<BoneTransform>& currentPose);

    // Spherical linear interpolation for quaternions
    Quaternion slerp(const Quaternion& q1, const Quaternion& q2, float t) const;

    // Cubic spline interpolation for smooth blending
    BoneTransform cubicSplineInterpolate(const std::vector<BoneTransform>& keyframes,
                                       float time, float duration) const;

    // Blend tree system
    class BlendTree {
    public:
        struct BlendTreeNode {
            enum Type { CLIP, BLEND_SPACE_1D, BLEND_SPACE_2D, ADD_NODE, LAYER };

            Type type;
            std::vector<std::unique_ptr<BlendTreeNode>> children;
            float weight;

            // 1D blend space
            struct BlendSpace1D {
                float parameter;
                std::vector<std::pair<float, AnimationClip*>> points;
            } blend1D;

            // 2D blend space
            struct BlendSpace2D {
                Vector2 parameter;
                std::vector<std::tuple<Vector2, AnimationClip*, float>> triangles;
            } blend2D;
        };

        BlendTreeNode* root;
        std::unordered_map<std::string, float> parameters;

        void evaluate(float dt, std::vector<BoneTransform>& output);
    };

    BlendTree blendTree;

    // Evaluate animation blending
    void evaluate(float dt, std::vector<BoneTransform>& output);
};

// Procedural Animation for non-human characters
class ProceduralAnimation {
public:
    struct ProceduralLimb {
        Vector3 rootPosition;
        std::vector<Vector3> jointPositions;
        std::vector<Quaternion> jointRotations;
        std::vector<float> lengths;

        // FABRIK IK solver
        bool solveFABRIK(const Vector3& target, float tolerance = 0.01f, int maxIterations = 10);
    };

    // Spider-like creature with 8 legs
    class SpiderProcedural {
        std::vector<ProceduralLimb> legs;
        Vector3 bodyPosition;
        Quaternion bodyRotation;

        void generateGaitPattern(float t);
        void applyTerrainAdaption(const std::vector<float>& terrainHeights);

        // Wave gait for smooth movement
        struct GaitPhase {
            float phase;     // 0-1
            float amplitude;
            float frequency;
        };
        std::vector<GaitPhase> legPhases;
    };

    // Quadruped procedural animation
    class QuadrupedProcedural {
        std::array<ProceduralLimb, 4> legs;
        Vector3 spinePosition;
        std::vector<Quaternion> spineJoints;

        void generateTrotingGait(float speed, float direction);
        void applySpineDeformation(const Vector3& movementVector);
    };

    // Flock animation (birds, fish)
    class FlockProcedural {
        struct Agent {
            Vector3 position;
            Vector3 velocity;
            Quaternion orientation;
            float scale;  // Size variation
        };

        std::vector<Agent> agents;
        std::vector<Vector3> flockForces;

        // Boids algorithm with skeletal animation
        void computeFlockForces();
        void applyWingFlapping(float time, float speed);
    };

    // Inverse Kinematics with multiple solutions
    class AdvancedIK {
    public:
        enum class IKSolverType { CCD, FABRIK, TWO_BONE, ANALYTICAL, HYBRID };

        struct IKSolution {
            std::vector<Quaternion> jointRotations;
            float error;          // Distance to target
            bool valid;
            int iterations;       // For performance tracking
        };

        // CCD (Cyclic Coordinate Descent)
        IKSolution solveCCD(const std::vector<Vector3>& jointPositions,
                          const std::vector<float>& boneLengths,
                          const Vector3& target,
                          const Vector3& poleVector = Vector3(0, 0, 0),
                          float tolerance = 0.01f,
                          int maxIterations = 50);

        // FABRIK with constraints
        IKSolution solveFABRIKConstrained(const std::vector<Vector3>& jointPositions,
                                         const std::vector<float>& boneLengths,
                                         const Vector3& target,
                                         const std::vector<std::pair<Vector3, Vector3>>& angleConstraints);

        // Multi-effector IK
        std::vector<IKSolution> solveMultiEffector(const std::vector<Vector3>& jointPositions,
                                                   const std::vector<Vector3>& targets,
                                                   const std::vector<float>& priorities);
    };

    AdvancedIK ikSolver;
};

// Facial Animation with blendshapes and phonemes
class FacialAnimation {
public:
    struct BlendShape {
        std::string name;
        std::vector<Vector3> vertexOffsets;
        float weight; // 0-1
    };

    struct FacialExpression {
        std::string name;
        std::unordered_map<std::string, float> blendWeights;
        std::vector<Vector2> phonemeInfluences; // Viseme positions
    };

    std::vector<BlendShape> blendShapes;

    // FACS (Facial Action Coding System)
    enum class FACSUnit {
        AU1_InnerBrowRaiser,
        AU2_OuterBrowRaiser,
        AU4_BrowLowerer,
        AU5_UpperLidRaiser,
        AU6_CheekRaiser,
        AU7_LidTightener,
        AU9_NoseWrinkler,
        AU10_UpperLipRaiser,
        AU11_LipCornerPuller,
        AU12_LipCornerDepressor,
        AU13_CheekPuffer,
        AU14_Dimpler,
        AU15_LipCornerDepressor,
        AU16_LowerLipDepressor,
        AU17_ChinRaiser,
        AU18_LipPuckerer,
        AU19_TongueShow,
        AU20_LipStretcher,
        AU21_NeckTightener,
        AU22_LipFunneler,
        AU23_LipTightener,
        AU24_LipPressor
    };

    std::unordered_map<FACSUnit, float> actionUnits;

    // Phoneme-based speech animation
    struct Phoneme {
        std::string symbol;
        std::vector<Vector2> visemePoints;  // Jaw, lip positions
        float duration;
        std::unordered_map<std::string, float> influence; // Mouth shape
    };

    std::vector<Phoneme> phonemes;

    // Text-to-speech viseme generation
    void generateVisemes(const std::string& text, float speakingRate);

    // Emotion-based facial rigging
    struct Emotion {
        enum Type { HAPPY, SAD, ANGRY, SURPRISED, FEARFUL, DISGUSTED, NEUTRAL };

        Type emotion;
        float intensity;
        std::vector<FACSUnit> dominantUnits;
    };

    void applyEmotion(const Emotion& emotion);

    // Performance capture retargeting
    void retargetMotionCapture(const std::vector<Vector3>& mocapData,
                             const std::vector<int>& faceLandmarks);
};

// Gesture Recognition for player input
class GestureRecognition {
public:
    enum class GestureType {
        POINT, WAVE, THUMBS_UP, PEACE_SIGN, FIST, OPEN_HAND,
        PINCH, SWIPE_LEFT, SWIPE_RIGHT, ROTATE, ZOOM
    };

    struct GestureData {
        GestureType type;
        Vector3 position;
        Vector3 direction;
        float confidence;
        float duration;
    };

    // Hand tracking
    struct HandSkeleton {
        std::array<Vector3, 26> jointPositions; // 21 finger + 5 palm joints

        // Finger curl detection
        std::array<float, 5> fingerCurls; // Thumb to Pinky
        Quaternion palmOrientation;
    };

    // Motion analysis
    GestureData detectGesture(const std::vector<HandSkeleton>& handHistory,
                            const Vector3& armDirection, float dt);

    // Pattern matching for complex gestures
    struct GesturePattern {
        std::string name;
        std::vector<std::vector<Vector3>> trajectorySamples; // Multiple examples
        GestureType type;
        float threshold; // Minimum confidence
    };

    std::vector<GesturePattern> gestureDatabase;

    // Probabilistic gesture recognition using HMM
    struct HMMState {
        std::string state;
        std::unordered_map<std::string, float> transitions;
        std::vector<Vector3> observationProbabilities;
    };

    std::vector<HMMState> gestureHMM;
    GestureData recognizeWithHMM(const std::vector<Vector3>& trajectory);

    // Haptic feedback for gesture confirmation
    void triggerHapticFeedback(GestureType gesture, float intensity);
};

// Performance Capture Integration
class MotionCaptureSystem {
public:
    struct MoCapData {
        std::vector<Vector3> markerPositions;
        std::vector<Vector3> markerVelocities;
        std::vector<Quaternion> jointRotations;
        uint64_t timestamp;
        float confidence;
    };

    struct SkeletonDefinition {
        std::vector<std::string> boneNames;
        std::vector<std::pair<int, int>> boneConnections; // parent, child indices
        std::vector<float> boneLengths;
    };

    SkeletonDefinition characterSkeleton;
    std::vector<MoCapData> captureFrames;

    // Optical marker tracking
    void trackMarkers(const std::vector<Vector3>& rawMarkerPositions,
                     std::vector<Vector3>& filteredPositions);

    // Skeleton solving from markers
    void solveSkeleton(const std::vector<Vector3>& markerPositions,
                      std::vector<Quaternion>& boneRotations,
                      std::vector<Vector3>& bonePositions);

    // Motion cleanup and stabilization
    void cleanMotionData(std::vector<MoCapData>& motionData);

    // Retargeting to different character proportions
    void retargetToCharacter(const std::vector<MoCapData>& sourceData,
                           const SkeletonDefinition& targetSkeleton,
                           std::vector<MoCapData>& retargetedData);
};

// Animation System Orchestrator
class AnimationEngine : public System {
public:
    DualQuaternionSkinning skinning;
    AnimationBlender blender;
    ProceduralAnimation proceduralAnim;
    FacialAnimation facialAnim;
    GestureRecognition gestures;
    MotionCaptureSystem mocap;

    // Character rig definition
    struct CharacterRig {
        std::string name;
        int boneCount;
        std::vector<std::string> boneNames;
        std::vector<Matrix4> bindPoses;
        std::vector<int> parentIndices;

        // IK constraints
        std::vector<std::pair<int, Quaternion>> rotationLimits; // Bone index, constraint
        std::vector<Vector3> ikTargets;
    };

    std::unordered_map<uint32_t, CharacterRig> characterRigs;

    // Animation state machine
    struct AnimationState {
        std::string name;
        AnimationBlender::AnimationClip* clip;
        std::unordered_map<std::string, std::string> transitions;

        // State conditions
        std::function<bool()> entryCondition;
        std::function<bool()> exitCondition;
    };

    struct AnimationController {
        uint32_t entityId;
        std::string currentState;
        std::unordered_map<std::string, AnimationState> states;
        std::unordered_map<std::string, float> parameters;

        void transitionTo(const std::string& stateName);
        void update(float dt);
    };

    std::unordered_map<uint32_t, AnimationController> controllers;

    void initialize();
    void update(float dt);

    // Animation event system
    struct AnimationEvent {
        std::string eventName;
        float timeStamp;
        uint32_t entityId;
    };

    std::vector<AnimationEvent> eventQueue;

    // Blend between procedural and keyframed animation
    void blendProceduralKeyframe(float proceduralWeight, uint32_t entityId);

    // Crowd animation optimization
    void optimizeForCrowd(std::vector<uint32_t>& crowdEntities,
                         const Vector3& cameraPosition,
                         float lodDistance);
};

} // namespace NeutralGameEngine

#endif // NEUTRAL_GAMEENGINE_ADVANCED_ANIMATION_H
