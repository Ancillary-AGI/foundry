#include "gtest/gtest.h"
#include "GameEngine/animation/AdvancedAnimation.h"
#include "GameEngine/animation/Kinematics.h"
#include "GameEngine/core/MemoryPool.h"
#include "GameEngine/math/Vector3.h"
#include "GameEngine/math/Quaternion.h"
#include "GameEngine/math/Matrix4.h"
#include <thread>
#include <chrono>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Animation Systems tests
 */
class AnimationSystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(2048, 16384);
    }

    void TearDown() override {
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
};

/**
 * @brief Test advanced animation system
 */
TEST_F(AnimationSystemsTest, AdvancedAnimation) {
    AdvancedAnimation animation;

    // Test animation initialization
    EXPECT_TRUE(animation.initialize());
    EXPECT_TRUE(animation.isInitialized());

    // Test animation clip creation
    AnimationClipID idleClip = animation.createAnimationClip("Idle");
    AnimationClipID walkClip = animation.createAnimationClip("Walk");
    AnimationClipID runClip = animation.createAnimationClip("Run");

    EXPECT_GT(idleClip, 0);
    EXPECT_GT(walkClip, 0);
    EXPECT_GT(runClip, 0);

    // Test animation clip properties
    animation.setClipDuration(idleClip, 2.0f);
    EXPECT_FLOAT_EQ(animation.getClipDuration(idleClip), 2.0f);

    animation.setClipLooping(idleClip, true);
    EXPECT_TRUE(animation.isClipLooping(idleClip));

    // Test keyframe management
    animation.addPositionKeyframe(idleClip, 0.0f, Vector3(0.0f, 0.0f, 0.0f));
    animation.addPositionKeyframe(idleClip, 1.0f, Vector3(0.1f, 0.0f, 0.0f));
    animation.addPositionKeyframe(idleClip, 2.0f, Vector3(0.0f, 0.0f, 0.0f));

    animation.addRotationKeyframe(idleClip, 0.0f, Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
    animation.addRotationKeyframe(idleClip, 2.0f, Quaternion(0.0f, 0.0f, 0.0f, 1.0f));

    animation.addScaleKeyframe(idleClip, 0.0f, Vector3(1.0f, 1.0f, 1.0f));
    animation.addScaleKeyframe(idleClip, 2.0f, Vector3(1.0f, 1.0f, 1.0f));

    EXPECT_EQ(animation.getPositionKeyframeCount(idleClip), 3);
    EXPECT_EQ(animation.getRotationKeyframeCount(idleClip), 2);
    EXPECT_EQ(animation.getScaleKeyframeCount(idleClip), 2);

    // Test animation state machine
    animation.createAnimationState("Locomotion");
    animation.addStateTransition("Locomotion", "Idle", "Walk", "Speed > 0.1");
    animation.addStateTransition("Locomotion", "Walk", "Run", "Speed > 2.0");
    animation.addStateTransition("Locomotion", "Run", "Walk", "Speed < 1.5");

    EXPECT_TRUE(animation.hasStateTransition("Locomotion", "Idle", "Walk"));

    // Test animation playback
    animation.playAnimation(idleClip);
    EXPECT_TRUE(animation.isAnimationPlaying());
    EXPECT_EQ(animation.getCurrentAnimation(), idleClip);

    animation.pauseAnimation();
    EXPECT_FALSE(animation.isAnimationPlaying());

    animation.resumeAnimation();
    EXPECT_TRUE(animation.isAnimationPlaying());

    animation.stopAnimation();
    EXPECT_FALSE(animation.isAnimationPlaying());

    // Test animation blending
    animation.setBlendWeight(walkClip, 0.5f);
    EXPECT_FLOAT_EQ(animation.getBlendWeight(walkClip), 0.5f);

    animation.enableAdditiveBlending(true);
    EXPECT_TRUE(animation.isAdditiveBlendingEnabled());

    // Test animation events
    animation.addAnimationEvent(idleClip, 1.0f, "Footstep");
    animation.addAnimationEvent(idleClip, 1.5f, "Footstep");

    EXPECT_EQ(animation.getAnimationEventCount(idleClip), 2);

    // Test animation controller
    animation.setAnimationParameter("Speed", 1.5f);
    EXPECT_FLOAT_EQ(animation.getAnimationParameter("Speed"), 1.5f);

    animation.setAnimationParameter("Direction", 0.7f);
    EXPECT_FLOAT_EQ(animation.getAnimationParameter("Direction"), 0.7f);

    // Test cleanup
    animation.destroyAnimationClip(runClip);
    animation.destroyAnimationClip(walkClip);
    animation.destroyAnimationClip(idleClip);

    animation.shutdown();
    EXPECT_FALSE(animation.isInitialized());
}

/**
 * @brief Test kinematics animation system
 */
TEST_F(AnimationSystemsTest, KinematicsAnimation) {
    KinematicsAnimation kinematics;

    // Test kinematics initialization
    EXPECT_TRUE(kinematics.initialize());
    EXPECT_TRUE(kinematics.isInitialized());

    // Test skeleton creation
    SkeletonID skeleton = kinematics.createSkeleton("Humanoid");
    EXPECT_GT(skeleton, 0);

    // Test bone hierarchy
    BoneID rootBone = kinematics.createBone("Root", Matrix4(), nullptr);
    BoneID spineBone = kinematics.createBone("Spine", Matrix4(), rootBone);
    BoneID leftArmBone = kinematics.createBone("LeftArm", Matrix4(), spineBone);
    BoneID rightArmBone = kinematics.createBone("RightArm", Matrix4(), spineBone);

    EXPECT_GT(rootBone, 0);
    EXPECT_GT(spineBone, 0);
    EXPECT_GT(leftArmBone, 0);
    EXPECT_GT(rightArmBone, 0);

    // Test bone properties
    kinematics.setBonePosition(rootBone, Vector3(0.0f, 0.0f, 0.0f));
    kinematics.setBoneRotation(rootBone, Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
    kinematics.setBoneScale(rootBone, Vector3(1.0f, 1.0f, 1.0f));

    Vector3 bonePos = kinematics.getBonePosition(rootBone);
    Quaternion boneRot = kinematics.getBoneRotation(rootBone);
    Vector3 boneScale = kinematics.getBoneScale(rootBone);

    EXPECT_EQ(bonePos, Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(boneRot, Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_EQ(boneScale, Vector3(1.0f, 1.0f, 1.0f));

    // Test bone constraints
    kinematics.addBoneConstraint(rootBone, ConstraintType::Position, Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
    kinematics.addBoneConstraint(rootBone, ConstraintType::Rotation, Vector3(-1.57f, -1.57f, -1.57f), Vector3(1.57f, 1.57f, 1.57f));

    EXPECT_GT(kinematics.getBoneConstraintCount(rootBone), 0);

    // Test pose calculation
    std::vector<Matrix4> boneTransforms = kinematics.calculateBoneTransforms(skeleton);
    EXPECT_GT(boneTransforms.size(), 0);

    // Test animation retargeting
    kinematics.enableRetargeting(true);
    EXPECT_TRUE(kinematics.isRetargetingEnabled());

    kinematics.setRetargetingRoot(spineBone);
    EXPECT_EQ(kinematics.getRetargetingRoot(), spineBone);

    // Test IK solving
    kinematics.enableIKSolving(true);
    EXPECT_TRUE(kinematics.isIKSolvingEnabled());

    kinematics.setIKEndEffector(leftArmBone, Vector3(1.0f, 0.0f, 0.0f));
    Vector3 endEffector = kinematics.getIKEndEffector(leftArmBone);
    EXPECT_EQ(endEffector, Vector3(1.0f, 0.0f, 0.0f));

    // Test animation blending
    kinematics.setBoneBlendWeight(leftArmBone, 0.7f);
    EXPECT_FLOAT_EQ(kinematics.getBoneBlendWeight(leftArmBone), 0.7f);

    // Test cleanup
    kinematics.destroyBone(rightArmBone);
    kinematics.destroyBone(leftArmBone);
    kinematics.destroyBone(spineBone);
    kinematics.destroyBone(rootBone);
    kinematics.destroySkeleton(skeleton);

    kinematics.shutdown();
    EXPECT_FALSE(kinematics.isInitialized());
}

/**
 * @brief Test animation performance
 */
TEST_F(AnimationSystemsTest, Performance) {
    const int numIterations = 100;

    // Measure animation performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        AdvancedAnimation animation;
        animation.initialize();

        // Create animation clip
        AnimationClipID clip = animation.createAnimationClip("PerfTest");
        animation.setClipDuration(clip, 1.0f);

        // Add keyframes
        for (int frame = 0; frame < 30; ++frame) {
            float time = frame / 30.0f;
            Vector3 position(static_cast<float>(i), static_cast<float>(frame), 0.0f);
            animation.addPositionKeyframe(clip, time, position);
        }

        // Play animation
        animation.playAnimation(clip);
        animation.updateAnimation(0.016f);

        animation.destroyAnimationClip(clip);
        animation.shutdown();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numIterations << " animation operations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 100ms for 100 operations)
    EXPECT_LT(duration.count(), 100000);
}

/**
 * @brief Test animation memory management
 */
TEST_F(AnimationSystemsTest, MemoryManagement) {
    size_t initialMemory = memoryPool->totalAllocated();

    // Create multiple animation systems to test memory usage
    std::vector<std::unique_ptr<AdvancedAnimation>> animations;
    std::vector<std::unique_ptr<KinematicsAnimation>> kinematicsSystems;

    for (int i = 0; i < 25; ++i) {
        auto animation = std::make_unique<AdvancedAnimation>();
        animation->initialize();

        AnimationClipID clip = animation->createAnimationClip("Clip" + std::to_string(i));
        animation->setClipDuration(clip, 2.0f);

        // Add multiple keyframes
        for (int frame = 0; frame < 10; ++frame) {
            float time = frame / 5.0f;
            animation->addPositionKeyframe(clip, time, Vector3(frame * 0.1f, 0.0f, 0.0f));
        }

        animations.push_back(std::move(animation));

        auto kinematics = std::make_unique<KinematicsAnimation>();
        kinematics->initialize();

        SkeletonID skeleton = kinematics->createSkeleton("Skeleton" + std::to_string(i));
        BoneID bone = kinematics->createBone("Bone" + std::to_string(i), Matrix4(), nullptr);
        kinematics->setBonePosition(bone, Vector3(i * 1.0f, 0.0f, 0.0f));

        kinematicsSystems.push_back(std::move(kinematics));
    }

    size_t afterAllocationMemory = memoryPool->totalAllocated();
    EXPECT_GT(afterAllocationMemory, initialMemory);

    // Test memory utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    // Clean up
    animations.clear();
    kinematicsSystems.clear();
}

/**
 * @brief Test animation error handling
 */
TEST_F(AnimationSystemsTest, ErrorHandling) {
    AdvancedAnimation animation;

    // Test invalid operations
    EXPECT_NO_THROW(animation.playAnimation(99999)); // Invalid clip ID should handle gracefully
    EXPECT_NO_THROW(animation.setBlendWeight(99999, 0.5f)); // Invalid clip ID should handle gracefully

    // Test uninitialized operations
    EXPECT_FALSE(animation.isInitialized());
    EXPECT_NO_THROW(animation.shutdown()); // Should handle multiple shutdowns

    // Test kinematics error handling
    KinematicsAnimation kinematics;
    EXPECT_NO_THROW(kinematics.setBonePosition(99999, Vector3(0.0f, 0.0f, 0.0f))); // Invalid bone ID should handle gracefully
    EXPECT_NO_THROW(kinematics.calculateBoneTransforms(99999)); // Invalid skeleton ID should handle gracefully
}

/**
 * @brief Test animation concurrent operations
 */
TEST_F(AnimationSystemsTest, ConcurrentOperations) {
    const int numThreads = 4;
    const int operationsPerThread = 25;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads performing animation operations
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&successCount, t]() {
            AdvancedAnimation animation;
            animation.initialize();

            for (int i = 0; i < operationsPerThread; ++i) {
                AnimationClipID clip = animation.createAnimationClip("ThreadClip" + std::to_string(t) + "_" + std::to_string(i));
                if (clip > 0) {
                    animation.setClipDuration(clip, 1.0f);
                    animation.addPositionKeyframe(clip, 0.0f, Vector3(t * 1.0f, i * 1.0f, 0.0f));
                    animation.addPositionKeyframe(clip, 0.5f, Vector3(t * 1.0f + 1.0f, i * 1.0f, 0.0f));
                    animation.addPositionKeyframe(clip, 1.0f, Vector3(t * 1.0f, i * 1.0f, 0.0f));

                    animation.playAnimation(clip);
                    animation.updateAnimation(0.016f);
                    animation.stopAnimation();

                    animation.destroyAnimationClip(clip);

                    successCount++;
                }
            }

            animation.shutdown();
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Verify concurrent operations worked
    EXPECT_EQ(successCount.load(), numThreads * operationsPerThread);

    // Memory pool should still be in valid state
    float utilization = memoryPool->utilization();
    EXPECT_GE(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);
}

/**
 * @brief Test animation state management
 */
TEST_F(AnimationSystemsTest, StateManagement) {
    AdvancedAnimation animation;
    animation.initialize();

    // Create multiple animation states
    animation.createAnimationState("Combat");
    animation.createAnimationState("Movement");
    animation.createAnimationState("Idle");

    // Test state transitions
    animation.addStateTransition("Idle", "Movement", "Movement", "Speed > 0");
    animation.addStateTransition("Movement", "Combat", "Combat", "InCombat = true");
    animation.addStateTransition("Combat", "Idle", "Idle", "InCombat = false");

    // Test state variables
    animation.setStateVariable("Idle", "Speed", 0.0f);
    animation.setStateVariable("Movement", "Speed", 1.5f);
    animation.setStateVariable("Combat", "Speed", 0.8f);

    EXPECT_FLOAT_EQ(animation.getStateVariable("Idle", "Speed"), 0.0f);
    EXPECT_FLOAT_EQ(animation.getStateVariable("Movement", "Speed"), 1.5f);
    EXPECT_FLOAT_EQ(animation.getStateVariable("Combat", "Speed"), 0.8f);

    // Test state activation
    animation.setActiveState("Movement");
    EXPECT_EQ(animation.getActiveState(), "Movement");

    animation.setActiveState("Combat");
    EXPECT_EQ(animation.getActiveState(), "Combat");

    // Test state blending
    animation.setStateBlendTime("Movement", 0.3f);
    EXPECT_FLOAT_EQ(animation.getStateBlendTime("Movement"), 0.3f);

    animation.enableStateBlending(true);
    EXPECT_TRUE(animation.isStateBlendingEnabled());

    animation.shutdown();
}

/**
 * @brief Test animation curve editing
 */
TEST_F(AnimationSystemsTest, CurveEditing) {
    AdvancedAnimation animation;
    animation.initialize();

    AnimationClipID clip = animation.createAnimationClip("CurveTest");
    animation.setClipDuration(clip, 2.0f);

    // Test curve keyframe editing
    animation.addPositionKeyframe(clip, 0.0f, Vector3(0.0f, 0.0f, 0.0f));
    animation.addPositionKeyframe(clip, 1.0f, Vector3(5.0f, 0.0f, 0.0f));
    animation.addPositionKeyframe(clip, 2.0f, Vector3(10.0f, 0.0f, 0.0f));

    // Test tangent editing
    animation.setKeyframeTangent(clip, 1.0f, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f));
    Vector3 inTangent, outTangent;
    animation.getKeyframeTangent(clip, 1.0f, inTangent, outTangent);
    EXPECT_EQ(inTangent, Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(outTangent, Vector3(0.0f, 0.0f, 0.0f));

    // Test interpolation modes
    animation.setInterpolationMode(clip, InterpolationMode::Cubic);
    EXPECT_EQ(animation.getInterpolationMode(clip), InterpolationMode::Cubic);

    animation.setInterpolationMode(clip, InterpolationMode::Linear);
    EXPECT_EQ(animation.getInterpolationMode(clip), InterpolationMode::Linear);

    // Test curve evaluation
    Vector3 positionAt1s = animation.evaluatePositionAtTime(clip, 1.0f);
    EXPECT_NEAR(positionAt1s.x, 5.0f, 0.1f);

    Vector3 positionAt0_5s = animation.evaluatePositionAtTime(clip, 0.5f);
    EXPECT_GT(positionAt0_5s.x, 0.0f);
    EXPECT_LT(positionAt0_5s.x, 5.0f);

    animation.destroyAnimationClip(clip);
    animation.shutdown();
}

/**
 * @brief Test animation compression
 */
TEST_F(AnimationSystemsTest, Compression) {
    AdvancedAnimation animation;
    animation.initialize();

    AnimationClipID clip = animation.createAnimationClip("CompressionTest");
    animation.setClipDuration(clip, 3.0f);

    // Add many keyframes for compression testing
    for (int i = 0; i < 100; ++i) {
        float time = i / 30.0f;
        Vector3 position(std::sin(time * 2.0f), std::cos(time * 2.0f), 0.0f);
        animation.addPositionKeyframe(clip, time, position);
    }

    EXPECT_EQ(animation.getPositionKeyframeCount(clip), 100);

    // Test compression
    animation.compressAnimation(clip, 0.01f); // 1cm tolerance
    int compressedKeyframes = animation.getPositionKeyframeCount(clip);
    EXPECT_LT(compressedKeyframes, 100); // Should have fewer keyframes after compression

    // Test compression quality
    Vector3 originalPos = animation.evaluatePositionAtTime(clip, 1.5f);

    // Should still be reasonably accurate
    EXPECT_GT(originalPos.x, -1.1f);
    EXPECT_LT(originalPos.x, 1.1f);
    EXPECT_GT(originalPos.y, -1.1f);
    EXPECT_LT(originalPos.y, 1.1f);

    animation.destroyAnimationClip(clip);
    animation.shutdown();
}

/**
 * @brief Test animation synchronization
 */
TEST_F(AnimationSystemsTest, Synchronization) {
    AdvancedAnimation animation;
    animation.initialize();

    // Create multiple animation clips
    AnimationClipID clip1 = animation.createAnimationClip("SyncClip1");
    AnimationClipID clip2 = animation.createAnimationClip("SyncClip2");

    animation.setClipDuration(clip1, 2.0f);
    animation.setClipDuration(clip2, 3.0f);

    // Test animation synchronization
    animation.syncAnimationClips(clip1, clip2);
    EXPECT_TRUE(animation.areClipsSynchronized(clip1, clip2));

    // Test time scaling
    animation.setTimeScale(clip1, 0.5f);
    EXPECT_FLOAT_EQ(animation.getTimeScale(clip1), 0.5f);

    animation.setTimeScale(clip2, 1.5f);
    EXPECT_FLOAT_EQ(animation.getTimeScale(clip2), 1.5f);

    // Test playback rate
    animation.setPlaybackRate(clip1, 2.0f);
    EXPECT_FLOAT_EQ(animation.getPlaybackRate(clip1), 2.0f);

    // Test animation events for synchronization
    animation.addAnimationEvent(clip1, 1.0f, "SyncPoint");
    animation.addAnimationEvent(clip2, 1.5f, "SyncPoint");

    EXPECT_EQ(animation.getAnimationEventCount(clip1), 1);
    EXPECT_EQ(animation.getAnimationEventCount(clip2), 1);

    animation.destroyAnimationClip(clip2);
    animation.destroyAnimationClip(clip1);
    animation.shutdown();
}

} // namespace Tests
} // namespace FoundryEngine
