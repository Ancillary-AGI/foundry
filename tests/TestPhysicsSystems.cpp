#include "gtest/gtest.h"
#include "GameEngine/systems/PhysicsSystem.h"
#include "GameEngine/systems/ClothSystem.h"
#include "GameEngine/systems/FluidSystem.h"
#include "GameEngine/systems/SpringSystem.h"
#include "GameEngine/systems/NBodySystem.h"
#include "GameEngine/systems/KinematicsSystem.h"
#include "GameEngine/systems/DopplerEffect.h"
#include "GameEngine/systems/DeformableBodySystem.h"
#include "GameEngine/systems/RealWorldPhysicsEnhancements.h"
#include "GameEngine/core/MemoryPool.h"
#include "GameEngine/math/Vector3.h"
#include "GameEngine/math/Quaternion.h"
#include "GameEngine/math/Matrix4.h"
#include <thread>
#include <chrono>

namespace FoundryEngine {
namespace Tests {

/**
 * @brief Test fixture for Physics Systems tests
 */
class PhysicsSystemsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memoryPool = std::make_unique<MemoryPool>(4096, 32768);
    }

    void TearDown() override {
        memoryPool.reset();
    }

    std::unique_ptr<MemoryPool> memoryPool;
};

/**
 * @brief Test physics system functionality
 */
TEST_F(PhysicsSystemsTest, PhysicsSystem) {
    PhysicsSystem physics;

    // Test physics initialization
    EXPECT_TRUE(physics.initialize());
    EXPECT_TRUE(physics.isInitialized());

    // Test gravity management
    physics.setGravity(Vector3(0.0f, -9.81f, 0.0f));
    Vector3 gravity = physics.getGravity();
    EXPECT_FLOAT_EQ(gravity.y, -9.81f);

    // Test world bounds
    physics.setWorldBounds(Vector3(-100.0f, -100.0f, -100.0f), Vector3(100.0f, 100.0f, 100.0f));
    Vector3 minBounds, maxBounds;
    physics.getWorldBounds(minBounds, maxBounds);
    EXPECT_EQ(minBounds, Vector3(-100.0f, -100.0f, -100.0f));
    EXPECT_EQ(maxBounds, Vector3(100.0f, 100.0f, 100.0f));

    // Test rigid body creation
    RigidBodyID body1 = physics.createRigidBody(1.0f, Vector3(0.0f, 0.0f, 0.0f));
    RigidBodyID body2 = physics.createRigidBody(2.0f, Vector3(5.0f, 0.0f, 0.0f));

    EXPECT_NE(body1, body2);
    EXPECT_GT(body1, 0);
    EXPECT_GT(body2, 0);

    // Test rigid body properties
    physics.setRigidBodyPosition(body1, Vector3(0.0f, 10.0f, 0.0f));
    physics.setRigidBodyVelocity(body1, Vector3(1.0f, 0.0f, 0.0f));
    physics.setRigidBodyMass(body1, 5.0f);

    Vector3 position = physics.getRigidBodyPosition(body1);
    Vector3 velocity = physics.getRigidBodyVelocity(body1);
    float mass = physics.getRigidBodyMass(body1);

    EXPECT_EQ(position, Vector3(0.0f, 10.0f, 0.0f));
    EXPECT_EQ(velocity, Vector3(1.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(mass, 5.0f);

    // Test force application
    physics.applyForce(body1, Vector3(0.0f, 100.0f, 0.0f));
    physics.applyTorque(body1, Vector3(0.0f, 0.0f, 10.0f));

    // Test collision detection
    physics.enableCollisionDetection(true);
    EXPECT_TRUE(physics.isCollisionDetectionEnabled());

    physics.enableCollisionDetection(false);
    EXPECT_FALSE(physics.isCollisionDetectionEnabled());

    // Test physics simulation
    physics.setTimeStep(0.016f);
    EXPECT_FLOAT_EQ(physics.getTimeStep(), 0.016f);

    physics.stepSimulation();
    // Physics state should have been updated

    // Test cleanup
    physics.destroyRigidBody(body1);
    physics.destroyRigidBody(body2);

    physics.shutdown();
    EXPECT_FALSE(physics.isInitialized());
}

/**
 * @brief Test cloth simulation system
 */
TEST_F(PhysicsSystemsTest, ClothSystem) {
    ClothSystem cloth;

    // Test cloth initialization
    EXPECT_TRUE(cloth.initialize());
    EXPECT_TRUE(cloth.isInitialized());

    // Test cloth creation
    ClothID cloth1 = cloth.createCloth(10, 10, 1.0f);
    EXPECT_GT(cloth1, 0);

    // Test cloth properties
    cloth.setClothStiffness(cloth1, 0.8f);
    EXPECT_FLOAT_EQ(cloth.getClothStiffness(cloth1), 0.8f);

    cloth.setClothDamping(cloth1, 0.1f);
    EXPECT_FLOAT_EQ(cloth.getClothDamping(cloth1), 0.1f);

    cloth.setClothGravity(cloth1, Vector3(0.0f, -5.0f, 0.0f));
    Vector3 clothGravity = cloth.getClothGravity(cloth1);
    EXPECT_FLOAT_EQ(clothGravity.y, -5.0f);

    // Test cloth constraints
    cloth.addDistanceConstraint(cloth1, 0, 1, 1.0f);
    cloth.addBendingConstraint(cloth1, 0, 1, 2, 0.5f);

    EXPECT_GT(cloth.getConstraintCount(cloth1), 0);

    // Test cloth animation
    cloth.setWindForce(cloth1, Vector3(1.0f, 0.0f, 0.0f));
    Vector3 windForce = cloth.getWindForce(cloth1);
    EXPECT_FLOAT_EQ(windForce.x, 1.0f);

    // Test cloth collision
    cloth.enableSelfCollision(cloth1, true);
    EXPECT_TRUE(cloth.hasSelfCollisionEnabled(cloth1));

    // Test cloth simulation
    cloth.setTimeStep(0.016f);
    cloth.simulateStep();

    // Test cleanup
    cloth.destroyCloth(cloth1);
    cloth.shutdown();
    EXPECT_FALSE(cloth.isInitialized());
}

/**
 * @brief Test fluid simulation system
 */
TEST_F(PhysicsSystemsTest, FluidSystem) {
    FluidSystem fluid;

    // Test fluid initialization
    EXPECT_TRUE(fluid.initialize());
    EXPECT_TRUE(fluid.isInitialized());

    // Test fluid creation
    FluidID fluid1 = fluid.createFluid(1000, Vector3(0.0f, -5.0f, 0.0f));
    EXPECT_GT(fluid1, 0);

    // Test fluid properties
    fluid.setFluidDensity(fluid1, 1000.0f);
    EXPECT_FLOAT_EQ(fluid.getFluidDensity(fluid1), 1000.0f);

    fluid.setFluidViscosity(fluid1, 0.001f);
    EXPECT_FLOAT_EQ(fluid.getFluidViscosity(fluid1), 0.001f);

    fluid.setFluidSurfaceTension(fluid1, 0.07f);
    EXPECT_FLOAT_EQ(fluid.getFluidSurfaceTension(fluid1), 0.07f);

    // Test fluid emitters
    EmitterID emitter1 = fluid.createEmitter(Vector3(0.0f, 10.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f));
    EXPECT_GT(emitter1, 0);

    fluid.setEmitterRate(emitter1, 100.0f);
    EXPECT_FLOAT_EQ(fluid.getEmitterRate(emitter1), 100.0f);

    fluid.setEmitterRadius(emitter1, 0.5f);
    EXPECT_FLOAT_EQ(fluid.getEmitterRadius(emitter1), 0.5f);

    // Test fluid simulation
    fluid.setTimeStep(0.016f);
    fluid.setSPHParameters(16, 0.02f, 0.04f);

    fluid.simulateStep();

    // Test fluid rendering
    fluid.enableRendering(fluid1, true);
    EXPECT_TRUE(fluid.isRenderingEnabled(fluid1));

    fluid.setParticleSize(fluid1, 0.02f);
    EXPECT_FLOAT_EQ(fluid.getParticleSize(fluid1), 0.02f);

    // Test cleanup
    fluid.destroyEmitter(emitter1);
    fluid.destroyFluid(fluid1);
    fluid.shutdown();
    EXPECT_FALSE(fluid.isInitialized());
}

/**
 * @brief Test spring system
 */
TEST_F(PhysicsSystemsTest, SpringSystem) {
    SpringSystem springs;

    // Test spring initialization
    EXPECT_TRUE(springs.initialize());
    EXPECT_TRUE(springs.isInitialized());

    // Test spring creation
    SpringID spring1 = springs.createSpring(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
    EXPECT_GT(spring1, 0);

    // Test spring properties
    springs.setSpringStiffness(spring1, 100.0f);
    EXPECT_FLOAT_EQ(springs.getSpringStiffness(spring1), 100.0f);

    springs.setSpringDamping(spring1, 0.1f);
    EXPECT_FLOAT_EQ(springs.getSpringDamping(spring1), 0.1f);

    springs.setSpringRestLength(spring1, 1.5f);
    EXPECT_FLOAT_EQ(springs.getSpringRestLength(spring1), 1.5f);

    // Test spring forces
    springs.setSpringForce(spring1, Vector3(0.0f, 10.0f, 0.0f));
    Vector3 springForce = springs.getSpringForce(spring1);
    EXPECT_FLOAT_EQ(springForce.y, 10.0f);

    // Test spring constraints
    springs.enableSpringConstraint(spring1, true);
    EXPECT_TRUE(springs.isSpringConstrained(spring1));

    // Test spring simulation
    springs.setTimeStep(0.016f);
    springs.simulateStep();

    // Test cleanup
    springs.destroySpring(spring1);
    springs.shutdown();
    EXPECT_FALSE(springs.isInitialized());
}

/**
 * @brief Test N-body simulation system
 */
TEST_F(PhysicsSystemsTest, NBodySystem) {
    NBodySystem nbody;

    // Test N-body initialization
    EXPECT_TRUE(nbody.initialize());
    EXPECT_TRUE(nbody.isInitialized());

    // Test body creation
    BodyID body1 = nbody.createBody(1e12f, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f));
    BodyID body2 = nbody.createBody(1e10f, Vector3(10.0f, 0.0f, 0.0f), Vector3(0.0f, 5.0f, 0.0f));

    EXPECT_NE(body1, body2);
    EXPECT_GT(body1, 0);
    EXPECT_GT(body2, 0);

    // Test gravitational constant
    nbody.setGravitationalConstant(6.67430e-11f);
    EXPECT_FLOAT_EQ(nbody.getGravitationalConstant(), 6.67430e-11f);

    // Test softening parameter
    nbody.setSofteningParameter(0.1f);
    EXPECT_FLOAT_EQ(nbody.getSofteningParameter(), 0.1f);

    // Test force calculation
    nbody.enableForceCalculation(true);
    EXPECT_TRUE(nbody.isForceCalculationEnabled());

    // Test integration method
    nbody.setIntegrationMethod(IntegrationMethod::Verlet);
    EXPECT_EQ(nbody.getIntegrationMethod(), IntegrationMethod::Verlet);

    // Test simulation
    nbody.setTimeStep(0.016f);
    nbody.simulateStep();

    // Test performance optimization
    nbody.enableBarnesHut(true);
    EXPECT_TRUE(nbody.isBarnesHutEnabled());

    nbody.setThetaThreshold(0.5f);
    EXPECT_FLOAT_EQ(nbody.getThetaThreshold(), 0.5f);

    // Test cleanup
    nbody.destroyBody(body1);
    nbody.destroyBody(body2);
    nbody.shutdown();
    EXPECT_FALSE(nbody.isInitialized());
}

/**
 * @brief Test kinematics system
 */
TEST_F(PhysicsSystemsTest, KinematicsSystem) {
    KinematicsSystem kinematics;

    // Test kinematics initialization
    EXPECT_TRUE(kinematics.initialize());
    EXPECT_TRUE(kinematics.isInitialized());

    // Test joint creation
    JointID joint1 = kinematics.createJoint(JointType::Revolute, Vector3(0.0f, 0.0f, 0.0f));
    JointID joint2 = kinematics.createJoint(JointType::Prismatic, Vector3(5.0f, 0.0f, 0.0f));

    EXPECT_NE(joint1, joint2);
    EXPECT_GT(joint1, 0);
    EXPECT_GT(joint2, 0);

    // Test joint properties
    kinematics.setJointLimits(joint1, -1.57f, 1.57f); // -90 to 90 degrees
    float minLimit, maxLimit;
    kinematics.getJointLimits(joint1, minLimit, maxLimit);
    EXPECT_FLOAT_EQ(minLimit, -1.57f);
    EXPECT_FLOAT_EQ(maxLimit, 1.57f);

    kinematics.setJointVelocity(joint1, 1.0f);
    EXPECT_FLOAT_EQ(kinematics.getJointVelocity(joint1), 1.0f);

    kinematics.setJointAcceleration(joint1, 0.5f);
    EXPECT_FLOAT_EQ(kinematics.getJointAcceleration(joint1), 0.5f);

    // Test inverse kinematics
    kinematics.enableInverseKinematics(true);
    EXPECT_TRUE(kinematics.isInverseKinematicsEnabled());

    kinematics.setIKTolerance(0.001f);
    EXPECT_FLOAT_EQ(kinematics.getIKTolerance(), 0.001f);

    kinematics.setIKMaxIterations(100);
    EXPECT_EQ(kinematics.getIKMaxIterations(), 100);

    // Test forward kinematics
    std::vector<float> jointAngles = {0.5f, -0.3f, 0.8f};
    Vector3 endEffectorPos = kinematics.computeForwardKinematics(jointAngles);
    // Should compute some position based on joint angles

    // Test jacobian calculation
    Matrix4 jacobian = kinematics.computeJacobian(jointAngles);
    EXPECT_EQ(jacobian.m[0], 1.0f); // Identity matrix for simple case

    // Test cleanup
    kinematics.destroyJoint(joint1);
    kinematics.destroyJoint(joint2);
    kinematics.shutdown();
    EXPECT_FALSE(kinematics.isInitialized());
}

/**
 * @brief Test doppler effect system
 */
TEST_F(PhysicsSystemsTest, DopplerEffect) {
    DopplerEffect doppler;

    // Test doppler initialization
    EXPECT_TRUE(doppler.initialize());
    EXPECT_TRUE(doppler.isInitialized());

    // Test sound source creation
    SourceID source1 = doppler.createSoundSource(Vector3(0.0f, 0.0f, 0.0f), Vector3(10.0f, 0.0f, 0.0f));
    EXPECT_GT(source1, 0);

    // Test sound source properties
    doppler.setSourceFrequency(source1, 440.0f); // A4 note
    EXPECT_FLOAT_EQ(doppler.getSourceFrequency(source1), 440.0f);

    doppler.setSourceSpeed(source1, 10.0f);
    EXPECT_FLOAT_EQ(doppler.getSourceSpeed(source1), 10.0f);

    // Test listener management
    doppler.setListenerPosition(Vector3(20.0f, 0.0f, 0.0f));
    doppler.setListenerVelocity(Vector3(0.0f, 0.0f, 0.0f));

    Vector3 listenerPos = doppler.getListenerPosition();
    EXPECT_EQ(listenerPos, Vector3(20.0f, 0.0f, 0.0f));

    // Test doppler calculation
    float observedFrequency = doppler.calculateDopplerShift(source1);
    EXPECT_GT(observedFrequency, 0.0f);

    // Test sound propagation
    doppler.setSpeedOfSound(343.0f); // Speed of sound in air
    EXPECT_FLOAT_EQ(doppler.getSpeedOfSound(), 343.0f);

    doppler.enableSoundAttenuation(true);
    EXPECT_TRUE(doppler.isSoundAttenuationEnabled());

    // Test cleanup
    doppler.destroySoundSource(source1);
    doppler.shutdown();
    EXPECT_FALSE(doppler.isInitialized());
}

/**
 * @brief Test deformable body system
 */
TEST_F(PhysicsSystemsTest, DeformableBodySystem) {
    DeformableBodySystem deformable;

    // Test deformable body initialization
    EXPECT_TRUE(deformable.initialize());
    EXPECT_TRUE(deformable.isInitialized());

    // Test soft body creation
    SoftBodyID softBody1 = deformable.createSoftBody(8, 8, 8); // 8x8x8 grid
    EXPECT_GT(softBody1, 0);

    // Test soft body properties
    deformable.setSoftBodyMass(softBody1, 1.0f);
    EXPECT_FLOAT_EQ(deformable.getSoftBodyMass(softBody1), 1.0f);

    deformable.setSoftBodyStiffness(softBody1, 0.8f);
    EXPECT_FLOAT_EQ(deformable.getSoftBodyStiffness(softBody1), 0.8f);

    deformable.setSoftBodyDamping(softBody1, 0.1f);
    EXPECT_FLOAT_EQ(deformable.getSoftBodyDamping(softBody1), 0.1f);

    // Test deformation parameters
    deformable.setPoissonRatio(softBody1, 0.3f);
    EXPECT_FLOAT_EQ(deformable.getPoissonRatio(softBody1), 0.3f);

    deformable.setYoungModulus(softBody1, 1000.0f);
    EXPECT_FLOAT_EQ(deformable.getYoungModulus(softBody1), 1000.0f);

    // Test volume preservation
    deformable.enableVolumePreservation(softBody1, true);
    EXPECT_TRUE(deformable.isVolumePreservationEnabled(softBody1));

    // Test self-collision
    deformable.enableSelfCollision(softBody1, true);
    EXPECT_TRUE(deformable.isSelfCollisionEnabled(softBody1));

    // Test simulation
    deformable.setTimeStep(0.016f);
    deformable.simulateStep();

    // Test cleanup
    deformable.destroySoftBody(softBody1);
    deformable.shutdown();
    EXPECT_FALSE(deformable.isInitialized());
}

/**
 * @brief Test real-world physics enhancements
 */
TEST_F(PhysicsSystemsTest, RealWorldPhysicsEnhancements) {
    RealWorldPhysicsEnhancements realWorld;

    // Test real-world physics initialization
    EXPECT_TRUE(realWorld.initialize());
    EXPECT_TRUE(realWorld.isInitialized());

    // Test air resistance
    realWorld.setAirDensity(1.225f); // kg/m³
    EXPECT_FLOAT_EQ(realWorld.getAirDensity(), 1.225f);

    realWorld.setDragCoefficient(0.47f); // Sphere
    EXPECT_FLOAT_EQ(realWorld.getDragCoefficient(), 0.47f);

    // Test buoyancy
    realWorld.setFluidDensity(1000.0f); // Water
    EXPECT_FLOAT_EQ(realWorld.getFluidDensity(), 1000.0f);

    realWorld.enableBuoyancy(true);
    EXPECT_TRUE(realWorld.isBuoyancyEnabled());

    // Test friction
    realWorld.setStaticFriction(0.6f);
    EXPECT_FLOAT_EQ(realWorld.getStaticFriction(), 0.6f);

    realWorld.setKineticFriction(0.4f);
    EXPECT_FLOAT_EQ(realWorld.getKineticFriction(), 0.4f);

    // Test rolling resistance
    realWorld.setRollingResistance(0.02f);
    EXPECT_FLOAT_EQ(realWorld.getRollingResistance(), 0.02f);

    // Test Magnus effect
    realWorld.enableMagnusEffect(true);
    EXPECT_TRUE(realWorld.isMagnusEffectEnabled());

    realWorld.setMagnusCoefficient(0.5f);
    EXPECT_FLOAT_EQ(realWorld.getMagnusCoefficient(), 0.5f);

    // Test Coriolis effect
    realWorld.enableCoriolisEffect(true);
    EXPECT_TRUE(realWorld.isCoriolisEffectEnabled());

    realWorld.setLatitude(45.0f); // degrees
    EXPECT_FLOAT_EQ(realWorld.getLatitude(), 45.0f);

    // Test cleanup
    realWorld.shutdown();
    EXPECT_FALSE(realWorld.isInitialized());
}

/**
 * @brief Test physics performance
 */
TEST_F(PhysicsSystemsTest, Performance) {
    const int numIterations = 50;

    // Measure physics simulation performance
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIterations; ++i) {
        PhysicsSystem physics;
        physics.initialize();

        // Create multiple rigid bodies
        std::vector<RigidBodyID> bodies;
        for (int j = 0; j < 10; ++j) {
            RigidBodyID body = physics.createRigidBody(1.0f, Vector3(j * 2.0f, 0.0f, 0.0f));
            bodies.push_back(body);
        }

        physics.stepSimulation();

        // Clean up
        for (RigidBodyID body : bodies) {
            physics.destroyRigidBody(body);
        }
        physics.shutdown();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Performed " << numIterations << " physics simulations in "
              << duration.count() << " microseconds" << std::endl;

    // Performance should be reasonable (less than 200ms for 50 simulations)
    EXPECT_LT(duration.count(), 200000);
}

/**
 * @brief Test physics memory management
 */
TEST_F(PhysicsSystemsTest, MemoryManagement) {
    size_t initialMemory = memoryPool->totalAllocated();

    // Create multiple physics objects to test memory usage
    PhysicsSystem physics;
    physics.initialize();

    std::vector<RigidBodyID> bodies;
    std::vector<JointID> joints;

    KinematicsSystem kinematics;
    kinematics.initialize();

    // Create many rigid bodies
    for (int i = 0; i < 100; ++i) {
        RigidBodyID body = physics.createRigidBody(1.0f, Vector3(i * 1.0f, 0.0f, 0.0f));
        bodies.push_back(body);

        if (i > 0) {
            JointID joint = kinematics.createJoint(JointType::Revolute, Vector3(i * 1.0f, 0.0f, 0.0f));
            joints.push_back(joint);
        }
    }

    size_t afterAllocationMemory = memoryPool->totalAllocated();
    EXPECT_GT(afterAllocationMemory, initialMemory);

    // Test memory utilization
    float utilization = memoryPool->utilization();
    EXPECT_GT(utilization, 0.0f);
    EXPECT_LE(utilization, 100.0f);

    // Clean up
    for (JointID joint : joints) {
        kinematics.destroyJoint(joint);
    }
    joints.clear();

    for (RigidBodyID body : bodies) {
        physics.destroyRigidBody(body);
    }
    bodies.clear();

    kinematics.shutdown();
    physics.shutdown();
}

/**
 * @brief Test physics error handling
 */
TEST_F(PhysicsSystemsTest, ErrorHandling) {
    PhysicsSystem physics;

    // Test invalid operations
    EXPECT_NO_THROW(physics.setGravity(Vector3(0.0f, 0.0f, 0.0f))); // Zero gravity should be OK
    EXPECT_NO_THROW(physics.setTimeStep(-0.016f)); // Negative timestep should handle gracefully
    EXPECT_NO_THROW(physics.setTimeStep(0.0f)); // Zero timestep should handle gracefully

    // Test uninitialized operations
    EXPECT_FALSE(physics.isInitialized());
    EXPECT_NO_THROW(physics.shutdown()); // Should handle multiple shutdowns

    // Test invalid body operations
    EXPECT_NO_THROW(physics.destroyRigidBody(99999)); // Invalid body ID should handle gracefully
    EXPECT_NO_THROW(physics.setRigidBodyPosition(99999, Vector3(0.0f, 0.0f, 0.0f)));
}

/**
 * @brief Test physics concurrent operations
 */
TEST_F(PhysicsSystemsTest, ConcurrentOperations) {
    const int numThreads = 4;
    const int operationsPerThread = 25;

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Launch multiple threads performing physics operations
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&successCount, t]() {
            PhysicsSystem physics;
            physics.initialize();

            for (int i = 0; i < operationsPerThread; ++i) {
                RigidBodyID body = physics.createRigidBody(1.0f, Vector3(t * 10.0f, i * 1.0f, 0.0f));
                if (body > 0) {
                    physics.setRigidBodyVelocity(body, Vector3(1.0f, 0.0f, 0.0f));
                    physics.applyForce(body, Vector3(0.0f, 10.0f, 0.0f));
                    physics.destroyRigidBody(body);
                    successCount++;
                }
            }

            physics.shutdown();
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

} // namespace Tests
} // namespace FoundryEngine
