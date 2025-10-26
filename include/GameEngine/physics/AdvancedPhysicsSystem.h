/**
 * @file AdvancedPhysicsSystem.h
 * @brief Advanced physics simulation with fluid dynamics, soft bodies, and wave physics
 * @author FoundryEngine Team
 * @date 2024
 * @version 2.0.0
 *
 * This file contains the next-generation physics system for FoundryEngine.
 * Features include real-time fluid simulation, advanced cloth physics, soft body
 * dynamics, wave physics, destruction simulation, and GPU-accelerated particles.
 *
 * Key Features:
 * - Real-time fluid simulation using SPH and grid-based methods
 * - Advanced cloth physics with position-based dynamics
 * - Soft body physics with finite element methods
 * - Ocean and wave simulation with FFT-based techniques
 * - Real-time destruction and fracturing
 * - GPU-accelerated particle systems
 * - Multi-threaded rigid body dynamics
 * - Advanced constraint solving
 */

#pragma once

#include "../core/System.h"
#include "../math/Vector3.h"
#include "../math/Vector4.h"
#include "../math/Matrix4.h"
#include "../math/Quaternion.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>

namespace FoundryEngine {

// Forward declarations
class RigidBody;
class SoftBody;
class FluidSimulation;
class ClothSimulation;
class ParticleSystem;
class WaveSimulation;
class DestructionSystem;
class PhysicsWorld;
class CollisionShape;
class Constraint;

/**
 * @brief Physics simulation backend types
 */
enum class PhysicsBackend {
    Bullet,         ///< Bullet Physics (default)
    PhysX,          ///< NVIDIA PhysX
    Havok,          ///< Havok Physics
    Custom          ///< Custom implementation
};

/**
 * @brief Fluid simulation methods
 */
enum class FluidMethod {
    SPH,            ///< Smoothed Particle Hydrodynamics
    FLIP,           ///< Fluid-Implicit-Particle
    PIC,            ///< Particle-In-Cell
    LBM,            ///< Lattice Boltzmann Method
    MPM             ///< Material Point Method
};

/**
 * @brief Cloth simulation methods
 */
enum class ClothMethod {
    MassSpring,     ///< Mass-spring system
    PBD,            ///< Position-based dynamics
    FEM,            ///< Finite element method
    XPBD            ///< Extended position-based dynamics
};

/**
 * @brief Physics simulation quality levels
 */
enum class PhysicsQuality {
    Low,            ///< Low quality for mobile/low-end devices
    Medium,         ///< Medium quality for mainstream devices
    High,           ///< High quality for high-end devices
    Ultra           ///< Ultra quality for maximum accuracy
};

/**
 * @brief Physics world configuration
 */
struct PhysicsWorldConfig {
    Vector3 gravity = Vector3(0.0f, -9.81f, 0.0f);     ///< World gravity
    float timeStep = 1.0f / 60.0f;                      ///< Fixed time step
    int maxSubSteps = 10;                               ///< Maximum sub-steps per frame
    int solverIterations = 10;                          ///< Constraint solver iterations
    int velocityIterations = 8;                         ///< Velocity solver iterations
    float sleepThreshold = 0.8f;                        ///< Sleep threshold for rigid bodies
    float contactBreakingThreshold = 0.02f;             ///< Contact breaking threshold
    bool enableCCD = true;                              ///< Enable continuous collision detection
    bool enableWarmStarting = true;                     ///< Enable warm starting for constraints
    bool enableFriction = true;                         ///< Enable friction simulation
    bool enableRestitution = true;                      ///< Enable restitution (bouncing)
    PhysicsQuality quality = PhysicsQuality::High;      ///< Simulation quality
    size_t maxRigidBodies = 10000;                      ///< Maximum rigid bodies
    size_t maxSoftBodies = 100;                         ///< Maximum soft bodies
    size_t maxParticles = 1000000;                      ///< Maximum particles
    size_t maxConstraints = 50000;                      ///< Maximum constraints
};

/**
 * @brief Physics performance statistics
 */
struct PhysicsStatistics {
    float simulationTime = 0.0f;                        ///< Total simulation time (ms)
    float rigidBodyTime = 0.0f;                         ///< Rigid body simulation time (ms)
    float softBodyTime = 0.0f;                          ///< Soft body simulation time (ms)
    float fluidTime = 0.0f;                             ///< Fluid simulation time (ms)
    float clothTime = 0.0f;                             ///< Cloth simulation time (ms)
    float particleTime = 0.0f;                          ///< Particle simulation time (ms)
    float collisionTime = 0.0f;                         ///< Collision detection time (ms)
    float constraintTime = 0.0f;                        ///< Constraint solving time (ms)
    
    uint32_t activeRigidBodies = 0;                     ///< Number of active rigid bodies
    uint32_t activeSoftBodies = 0;                      ///< Number of active soft bodies
    uint32_t activeParticles = 0;                       ///< Number of active particles
    uint32_t activeConstraints = 0;                     ///< Number of active constraints
    uint32_t collisionPairs = 0;                        ///< Number of collision pairs
    uint32_t contactPoints = 0;                         ///< Number of contact points
    
    uint64_t memoryUsage = 0;                           ///< Memory usage in bytes
    float averageFrameTime = 0.0f;                      ///< Average frame time
    float peakFrameTime = 0.0f;                         ///< Peak frame time
};

/**
 * @class AdvancedPhysicsSystem
 * @brief Next-generation physics system with advanced simulation capabilities
 */
class AdvancedPhysicsSystem : public System {
public:
    AdvancedPhysicsSystem();
    ~AdvancedPhysicsSystem();

    // System interface
    bool initialize() override;
    bool initialize(PhysicsBackend backend, const PhysicsWorldConfig& config = PhysicsWorldConfig{});
    void shutdown() override;
    void update(float deltaTime) override;

    // World management
    PhysicsWorld* getPhysicsWorld() const { return physicsWorld_.get(); }
    void setWorldConfig(const PhysicsWorldConfig& config);
    const PhysicsWorldConfig& getWorldConfig() const { return config_; }

    // Rigid body physics
    std::shared_ptr<RigidBody> createRigidBody(std::shared_ptr<CollisionShape> shape, 
                                              float mass = 1.0f, 
                                              const Vector3& position = Vector3::zero(),
                                              const Quaternion& rotation = Quaternion::identity());
    void removeRigidBody(std::shared_ptr<RigidBody> body);
    std::vector<std::shared_ptr<RigidBody>> getAllRigidBodies() const;

    // Soft body physics
    std::shared_ptr<SoftBody> createSoftBody(const std::vector<Vector3>& vertices,
                                            const std::vector<uint32_t>& indices,
                                            float mass = 1.0f);
    std::shared_ptr<SoftBody> createCloth(uint32_t resX, uint32_t resY, 
                                         const Vector3& corner1, const Vector3& corner2,
                                         const Vector3& corner3, const Vector3& corner4);
    std::shared_ptr<SoftBody> createRope(const Vector3& start, const Vector3& end, 
                                        uint32_t segments, float mass = 1.0f);
    void removeSoftBody(std::shared_ptr<SoftBody> body);

    // Fluid simulation
    std::shared_ptr<FluidSimulation> createFluidSimulation(FluidMethod method = FluidMethod::SPH);
    void removeFluidSimulation(std::shared_ptr<FluidSimulation> fluid);
    std::vector<std::shared_ptr<FluidSimulation>> getAllFluidSimulations() const;

    // Cloth simulation
    std::shared_ptr<ClothSimulation> createClothSimulation(ClothMethod method = ClothMethod::PBD);
    void removeClothSimulation(std::shared_ptr<ClothSimulation> cloth);

    // Particle systems
    std::shared_ptr<ParticleSystem> createParticleSystem(uint32_t maxParticles = 10000);
    void removeParticleSystem(std::shared_ptr<ParticleSystem> particles);

    // Wave simulation
    std::shared_ptr<WaveSimulation> createWaveSimulation(uint32_t resolution = 512);
    void removeWaveSimulation(std::shared_ptr<WaveSimulation> waves);

    // Destruction system
    DestructionSystem* getDestructionSystem() const { return destructionSystem_.get(); }
    void enableDestruction(bool enable) { destructionEnabled_ = enable; }
    bool isDestructionEnabled() const { return destructionEnabled_; }

    // Constraints
    std::shared_ptr<Constraint> createPointToPointConstraint(std::shared_ptr<RigidBody> bodyA,
                                                            std::shared_ptr<RigidBody> bodyB,
                                                            const Vector3& pivotA,
                                                            const Vector3& pivotB);
    std::shared_ptr<Constraint> createHingeConstraint(std::shared_ptr<RigidBody> bodyA,
                                                     std::shared_ptr<RigidBody> bodyB,
                                                     const Vector3& pivotA, const Vector3& pivotB,
                                                     const Vector3& axisA, const Vector3& axisB);
    std::shared_ptr<Constraint> createSliderConstraint(std::shared_ptr<RigidBody> bodyA,
                                                      std::shared_ptr<RigidBody> bodyB,
                                                      const Matrix4& frameA, const Matrix4& frameB);
    void removeConstraint(std::shared_ptr<Constraint> constraint);

    // Collision detection
    struct RaycastResult {
        bool hit = false;
        Vector3 point;
        Vector3 normal;
        float distance = 0.0f;
        std::shared_ptr<RigidBody> body;
    };
    
    RaycastResult raycast(const Vector3& from, const Vector3& to) const;
    std::vector<RaycastResult> raycastAll(const Vector3& from, const Vector3& to) const;
    
    struct SpherecastResult {
        bool hit = false;
        Vector3 point;
        Vector3 normal;
        float distance = 0.0f;
        std::shared_ptr<RigidBody> body;
    };
    
    SpherecastResult spherecast(const Vector3& from, const Vector3& to, float radius) const;
    
    std::vector<std::shared_ptr<RigidBody>> overlapSphere(const Vector3& center, float radius) const;
    std::vector<std::shared_ptr<RigidBody>> overlapBox(const Vector3& center, const Vector3& halfExtents) const;

    // Collision callbacks
    using CollisionCallback = std::function<void(std::shared_ptr<RigidBody>, std::shared_ptr<RigidBody>, const Vector3&)>;
    void setCollisionEnterCallback(CollisionCallback callback) { collisionEnterCallback_ = callback; }
    void setCollisionExitCallback(CollisionCallback callback) { collisionExitCallback_ = callback; }
    void setCollisionStayCallback(CollisionCallback callback) { collisionStayCallback_ = callback; }

    // Debug rendering
    void setDebugDrawEnabled(bool enabled) { debugDrawEnabled_ = enabled; }
    bool isDebugDrawEnabled() const { return debugDrawEnabled_; }
    void debugDrawWorld();

    // Performance and statistics
    const PhysicsStatistics& getStatistics() const { return statistics_; }
    void resetStatistics();
    
    // Threading
    void setThreadCount(uint32_t threadCount);
    uint32_t getThreadCount() const { return threadCount_; }

    // GPU acceleration
    void enableGPUAcceleration(bool enable) { gpuAcceleration_ = enable; }
    bool isGPUAccelerationEnabled() const { return gpuAcceleration_; }

private:
    class AdvancedPhysicsSystemImpl;
    std::unique_ptr<AdvancedPhysicsSystemImpl> impl_;

    PhysicsBackend backend_ = PhysicsBackend::Bullet;
    PhysicsWorldConfig config_;
    
    std::unique_ptr<PhysicsWorld> physicsWorld_;
    std::unique_ptr<DestructionSystem> destructionSystem_;
    
    std::vector<std::shared_ptr<RigidBody>> rigidBodies_;
    std::vector<std::shared_ptr<SoftBody>> softBodies_;
    std::vector<std::shared_ptr<FluidSimulation>> fluidSimulations_;
    std::vector<std::shared_ptr<ClothSimulation>> clothSimulations_;
    std::vector<std::shared_ptr<ParticleSystem>> particleSystems_;
    std::vector<std::shared_ptr<WaveSimulation>> waveSimulations_;
    std::vector<std::shared_ptr<Constraint>> constraints_;

    PhysicsStatistics statistics_;
    CollisionCallback collisionEnterCallback_;
    CollisionCallback collisionExitCallback_;
    CollisionCallback collisionStayCallback_;

    std::atomic<bool> debugDrawEnabled_{false};
    std::atomic<bool> destructionEnabled_{false};
    std::atomic<bool> gpuAcceleration_{false};
    std::atomic<uint32_t> threadCount_{std::thread::hardware_concurrency()};

    mutable std::mutex physicsMutex_;
    std::vector<std::thread> workerThreads_;

    // Internal methods
    void initializeBackend();
    void shutdownBackend();
    void updateRigidBodies(float deltaTime);
    void updateSoftBodies(float deltaTime);
    void updateFluids(float deltaTime);
    void updateCloth(float deltaTime);
    void updateParticles(float deltaTime);
    void updateWaves(float deltaTime);
    void updateConstraints(float deltaTime);
    void handleCollisions();
    void updateStatistics();
};

/**
 * @class FluidSimulation
 * @brief Advanced fluid simulation with multiple methods
 */
class FluidSimulation {
public:
    /**
     * @brief Fluid properties
     */
    struct FluidProperties {
        float density = 1000.0f;                        ///< Fluid density (kg/m³)
        float viscosity = 0.001f;                       ///< Dynamic viscosity (Pa·s)
        float surfaceTension = 0.0728f;                 ///< Surface tension (N/m)
        float gasConstant = 2000.0f;                    ///< Gas constant for pressure
        float restDensity = 1000.0f;                    ///< Rest density
        float particleMass = 0.02f;                     ///< Particle mass
        float smoothingRadius = 0.1f;                   ///< SPH smoothing radius
        Vector3 externalForce = Vector3(0, -9.81f, 0);  ///< External forces (gravity)
    };

    /**
     * @brief Fluid boundary conditions
     */
    enum class BoundaryType {
        Open,           ///< Open boundary (particles can leave)
        Closed,         ///< Closed boundary (particles bounce)
        Periodic,       ///< Periodic boundary (wrap around)
        Absorbing       ///< Absorbing boundary (particles disappear)
    };

    FluidSimulation(FluidMethod method = FluidMethod::SPH);
    ~FluidSimulation();

    bool initialize(uint32_t maxParticles = 100000);
    void shutdown();
    void update(float deltaTime);

    // Particle management
    void addParticle(const Vector3& position, const Vector3& velocity = Vector3::zero());
    void addParticles(const std::vector<Vector3>& positions, const std::vector<Vector3>& velocities = {});
    void removeParticle(uint32_t index);
    void clearParticles();

    // Emitters
    void addEmitter(const Vector3& position, const Vector3& direction, float rate, float speed);
    void removeEmitter(uint32_t index);
    void clearEmitters();

    // Properties
    void setFluidProperties(const FluidProperties& properties) { properties_ = properties; }
    const FluidProperties& getFluidProperties() const { return properties_; }

    // Boundaries
    void setBoundary(const Vector3& min, const Vector3& max, BoundaryType type = BoundaryType::Closed);
    void addBoundaryPlane(const Vector3& point, const Vector3& normal);
    void addBoundarySphere(const Vector3& center, float radius);
    void addBoundaryBox(const Vector3& min, const Vector3& max);

    // Rendering data
    const std::vector<Vector3>& getParticlePositions() const { return positions_; }
    const std::vector<Vector3>& getParticleVelocities() const { return velocities_; }
    const std::vector<float>& getParticleDensities() const { return densities_; }
    const std::vector<float>& getParticlePressures() const { return pressures_; }
    uint32_t getParticleCount() const { return particleCount_; }

    // Advanced features
    void enableSurfaceReconstruction(bool enable) { surfaceReconstruction_ = enable; }
    void enableVorticity(bool enable) { vorticity_ = enable; }
    void enableTurbulence(bool enable) { turbulence_ = enable; }

private:
    FluidMethod method_;
    FluidProperties properties_;
    
    std::vector<Vector3> positions_;
    std::vector<Vector3> velocities_;
    std::vector<Vector3> forces_;
    std::vector<float> densities_;
    std::vector<float> pressures_;
    std::vector<float> masses_;
    
    uint32_t particleCount_ = 0;
    uint32_t maxParticles_ = 0;
    
    struct Emitter {
        Vector3 position;
        Vector3 direction;
        float rate;
        float speed;
        float timer;
    };
    std::vector<Emitter> emitters_;
    
    struct BoundaryPlane {
        Vector3 point;
        Vector3 normal;
    };
    std::vector<BoundaryPlane> boundaryPlanes_;
    
    bool surfaceReconstruction_ = false;
    bool vorticity_ = false;
    bool turbulence_ = false;
    
    // Method-specific implementations
    void updateSPH(float deltaTime);
    void updateFLIP(float deltaTime);
    void updatePIC(float deltaTime);
    void updateLBM(float deltaTime);
    void updateMPM(float deltaTime);
    
    void calculateDensityPressure();
    void calculateForces();
    void integrate(float deltaTime);
    void handleBoundaries();
    void updateNeighbors();
};

/**
 * @class WaveSimulation
 * @brief Advanced ocean and wave simulation using FFT
 */
class WaveSimulation {
public:
    /**
     * @brief Wave parameters based on Phillips spectrum
     */
    struct WaveParameters {
        float amplitude = 1.0f;                         ///< Wave amplitude
        Vector2 windDirection = Vector2(1.0f, 0.0f);    ///< Wind direction
        float windSpeed = 10.0f;                        ///< Wind speed (m/s)
        float fetch = 100000.0f;                        ///< Fetch distance (m)
        float depth = 1000.0f;                          ///< Water depth (m)
        float gravity = 9.81f;                          ///< Gravitational acceleration
        float damping = 0.001f;                         ///< Wave damping factor
        float choppiness = 1.0f;                        ///< Wave choppiness (Gerstner waves)
        float foamThreshold = 0.5f;                     ///< Foam generation threshold
    };

    WaveSimulation();
    ~WaveSimulation();

    bool initialize(uint32_t resolution = 512, float size = 1000.0f);
    void shutdown();
    void update(float deltaTime);

    // Wave configuration
    void setWaveParameters(const WaveParameters& params) { parameters_ = params; }
    const WaveParameters& getWaveParameters() const { return parameters_; }

    // Height field access
    float getHeightAt(float x, float z) const;
    Vector3 getNormalAt(float x, float z) const;
    Vector2 getDisplacementAt(float x, float z) const;
    
    // Rendering data
    const std::vector<float>& getHeightField() const { return heightField_; }
    const std::vector<Vector3>& getNormalField() const { return normalField_; }
    const std::vector<Vector2>& getDisplacementField() const { return displacementField_; }
    const std::vector<float>& getFoamField() const { return foamField_; }
    
    uint32_t getResolution() const { return resolution_; }
    float getSize() const { return size_; }

    // Advanced features
    void enableGerstnerWaves(bool enable) { gerstnerWaves_ = enable; }
    void enableFoamGeneration(bool enable) { foamGeneration_ = enable; }
    void enableCaustics(bool enable) { caustics_ = enable; }

private:
    WaveParameters parameters_;
    
    uint32_t resolution_;
    float size_;
    float time_ = 0.0f;
    
    std::vector<std::complex<float>> h0_;               // Initial wave amplitudes
    std::vector<std::complex<float>> h0Conj_;           // Complex conjugate of h0
    std::vector<std::complex<float>> heightSpectrum_;   // Height spectrum
    std::vector<std::complex<float>> displacementX_;    // X displacement spectrum
    std::vector<std::complex<float>> displacementZ_;    // Z displacement spectrum
    
    std::vector<float> heightField_;
    std::vector<Vector3> normalField_;
    std::vector<Vector2> displacementField_;
    std::vector<float> foamField_;
    
    bool gerstnerWaves_ = true;
    bool foamGeneration_ = true;
    bool caustics_ = false;
    
    void generateInitialSpectrum();
    void updateSpectrum(float deltaTime);
    void performFFT();
    void calculateNormals();
    void calculateFoam();
    
    float phillipsSpectrum(const Vector2& k) const;
    std::complex<float> gaussianRandom() const;
};

/**
 * @class DestructionSystem
 * @brief Real-time destruction and fracturing system
 */
class DestructionSystem {
public:
    /**
     * @brief Destruction methods
     */
    enum class DestructionMethod {
        Voronoi,        ///< Voronoi-based fracturing
        Delaunay,       ///< Delaunay triangulation
        Procedural,     ///< Procedural fracturing
        Precomputed     ///< Precomputed fracture patterns
    };

    /**
     * @brief Fracture parameters
     */
    struct FractureParameters {
        uint32_t maxFragments = 50;                     ///< Maximum number of fragments
        float minFragmentSize = 0.1f;                   ///< Minimum fragment size
        float impactThreshold = 10.0f;                  ///< Impact force threshold
        float fragmentLifetime = 30.0f;                 ///< Fragment lifetime (seconds)
        bool generateDebris = true;                     ///< Generate small debris particles
        bool enableSound = true;                        ///< Enable destruction sounds
        bool enableParticles = true;                    ///< Enable particle effects
    };

    DestructionSystem();
    ~DestructionSystem();

    bool initialize();
    void shutdown();
    void update(float deltaTime);

    // Destruction
    void destroyObject(std::shared_ptr<RigidBody> body, const Vector3& impactPoint, 
                      const Vector3& impactForce, DestructionMethod method = DestructionMethod::Voronoi);
    void precomputeFracture(std::shared_ptr<RigidBody> body, DestructionMethod method = DestructionMethod::Voronoi);

    // Configuration
    void setFractureParameters(const FractureParameters& params) { parameters_ = params; }
    const FractureParameters& getFractureParameters() const { return parameters_; }

    // Callbacks
    using DestructionCallback = std::function<void(std::shared_ptr<RigidBody>, const std::vector<std::shared_ptr<RigidBody>>&)>;
    void setDestructionCallback(DestructionCallback callback) { destructionCallback_ = callback; }

private:
    FractureParameters parameters_;
    DestructionCallback destructionCallback_;
    
    struct Fragment {
        std::shared_ptr<RigidBody> body;
        float lifetime;
        bool isDebris;
    };
    std::vector<Fragment> fragments_;
    
    std::vector<std::shared_ptr<RigidBody>> fractureVoronoi(std::shared_ptr<RigidBody> body, 
                                                           const Vector3& impactPoint, uint32_t numFragments);
    std::vector<std::shared_ptr<RigidBody>> fractureDelaunay(std::shared_ptr<RigidBody> body, 
                                                            const Vector3& impactPoint, uint32_t numFragments);
    std::vector<std::shared_ptr<RigidBody>> fractureProcedural(std::shared_ptr<RigidBody> body, 
                                                              const Vector3& impactPoint, uint32_t numFragments);
    
    void generateDebris(const Vector3& position, uint32_t count);
    void cleanupFragments();
};

} // namespace FoundryEngine