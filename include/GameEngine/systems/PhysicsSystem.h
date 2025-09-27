#ifndef NEUTRAL_GAMEENGINE_PHYSICS_SYSTEM_H
#define NEUTRAL_GAMEENGINE_PHYSICS_SYSTEM_H

#include <vector>
#include <unordered_map>
#include <memory>
#include "../../math/Vector3.h"
#include "../../math/Quaternion.h"
#include "../../math/Matrix4.h"
#include "../System.h"

namespace NeutralGameEngine {

// Rigid Body Dynamics
class RigidBody {
public:
    Vector3 position;
    Quaternion orientation;
    Vector3 linearVelocity;
    Vector3 angularVelocity;

    float mass = 1.0f;
    Matrix3 inertiaTensor;      // Inertia tensor (local space)
    Matrix3 inverseInertiaTensor; // Inverse inertia tensor (world space)

    bool isStatic = false;
    bool isKinematic = false;

    // Force and torque accumulators
    Vector3 forceAccumulator;
    Vector3 torqueAccumulator;

    // Collision shape
    enum class ShapeType { BOX, SPHERE, CAPSULE, CONVEX_HULL, MESH };
    ShapeType collisionShape;
    std::vector<Vector3> shapeData; // Vertices for convex hull/mesh

    // Material properties
    float restitution = 0.5f;   // Bounciness
    float friction = 0.5f;      // Friction coefficient

    // Constraints
    std::vector<std::shared_ptr<class Constraint>> constraints;

    void integrate(float dt);
    void clearAccumulators();
    void addForce(const Vector3& force);
    void addForceAtPoint(const Vector3& force, const Vector3& point);
    void addTorque(const Vector3& torque);

    // Get transform matrix
    Matrix4 getTransform() const;

    // Update inertia tensor in world space
    void updateInertiaTensor();
};

class Constraint {
public:
    RigidBody* bodyA;
    RigidBody* bodyB;

    // Constraint points in local space
    Vector3 localPointA;
    Vector3 localPointB;

    // Current constraint error
    Vector3 positionError;
    Vector3 orientationError;

    virtual void solve(float dt) = 0;
    virtual bool isSatisfied(float epsilon = 1e-6f) const = 0;
};

// Ball-and-socket joint
class BallSocketConstraint : public Constraint {
public:
    Vector3 anchorPoint; // World space anchor

    void solve(float dt) override;
    bool isSatisfied(float epsilon = 1e-6f) const override;
};

// Hinge joint
class HingeConstraint : public Constraint {
public:
    Vector3 hingeAxis; // Local to body A
    float minAngle = -3.14159f;
    float maxAngle = 3.14159f;

    void solve(float dt) override;
    bool isSatisfied(float epsilon = 1e-6f) const override;
};

// Prismatic joint (linear movement)
class PrismaticConstraint : public Constraint {
public:
    Vector3 slideAxis; // Local to body A
    float minDistance = 0.0f;
    float maxDistance = 1.0f;

    void solve(float dt) override;
    bool isSatisfied(float epsilon = 1e-6f) const override;
};

// Soft constraints with stabilization
class SoftConstraint : public Constraint {
public:
    float compliance = 0.0f; // Inverse stiffness (higher = softer)
    float damping = 1.0f;    // Damping coefficient

    void solve(float dt) override;
    bool isSatisfied(float epsilon = 1e-6f) const override;
};

// Continuous Collision Detection (CCD)
class CCD {
public:
    struct CollisionData {
        RigidBody* bodyA;
        RigidBody* bodyB;
        Vector3 contactPoint;
        Vector3 contactNormal;
        float penetration;
        float toi; // Time of impact
    };

    // Sweep test for two bodies
    bool sweepTest(RigidBody* bodyA, RigidBody* bodyB, float dt, CollisionData& data);

    // Conservative advancement
    bool conservativeAdvancement(RigidBody* bodyA, RigidBody* bodyB, float dt, CollisionData& data);

    // Minkowski sum computation for convex shapes
    std::vector<Vector3> computeCSO(const std::vector<Vector3>& shapeA, const std::vector<Vector3>& shapeB) const;

    // GJK algorithm
    bool GJK(const std::vector<Vector3>& cso, Vector3& simplex) const;

    // EPA for contact information
    void EPA(Vector3 simplex[4], const std::vector<Vector3>& cso, Vector3& contactPoint, Vector3& normal, float& penetration);
};

// Articulated Bodies and Rag Doll Physics
class ArticulatedBody {
public:
    std::vector<RigidBody*> bodies;
    std::vector<std::shared_ptr<Constraint>> joints;

    // Build ragdoll from skeleton
    void createRagdoll(const std::vector<Vector3>& bonePositions,
                      const std::vector<Quaternion>& boneOrientations);

    // Solve forward kinematics
    void solveFK();

    // Solve inverse kinematics
    void solveIK(const Vector3& target, float maxError = 0.01f, int maxIterations = 10);

    // Handle collision response for ragdoll
    void handleCollisions(const std::vector<CollisionData>& collisions);

private:
    // Kinematic chain representation
    std::vector<std::pair<int, int>> boneChain; // parent, child indices
    std::vector<Matrix4> localTransforms;
};

// Fracture/Destruction System
class FractureSystem {
public:
    struct VoronoiCell {
        Vector3 centroid;
        std::vector<Vector3> vertices;
        std::vector<int> neighbors;
    };

    // Voronoi fracture
    std::vector<VoronoiCell> voronoiFracture(const std::vector<Vector3>& meshVertices,
                                           const std::vector<int>& meshIndices,
                                           const std::vector<Vector3>& fracturePoints);

    // Create rigid bodies from fractured pieces
    std::vector<RigidBody*> createFracturedBodies(const std::vector<VoronoiCell>& cells,
                                                const Vector3& materialProperties);

    // Apply fracture forces
    void applyFractureForces(std::vector<RigidBody*>& pieces, const Vector3& impactPoint, float force);

private:
    // 3D Voronoi diagram computation
    std::vector<VoronoiCell> computeVoronoiDiagram(const std::vector<Vector3>& points, const Vector3& bounds);

    // Clip voronoi cells to mesh
    std::vector<VoronoiCell> clipCellsToMesh(const std::vector<VoronoiCell>& cells,
                                           const std::vector<Vector3>& meshVertices);
};

// Soft Body & Deformable Physics (FEM)
class DeformableBody {
public:
    struct Tetrahedron {
        int vertices[4];
        float volume;
        Matrix3 deformationGradient;
    };

    std::vector<Vector3> positions;      // Particle positions
    std::vector<Vector3> velocities;     // Particle velocities
    std::vector<float> masses;          // Particle masses
    std::vector<Tetrahedron> tetrahedra; // Tetrahedral elements

    // FEM parameters
    float youngsModulus = 1e6f;     // Material stiffness
    float poissonRatio = 0.3f;      // Poisson ratio
    float damping = 0.99f;          // Damping coefficient

    void initializeFEM(const std::vector<Vector3>& initialPositions,
                      const std::vector<std::vector<int>>& tetIndices);

    void stepFEM(float dt);

    // Compute strain energy
    float computeStrainEnergy() const;

    // Apply external forces
    void applyForces(const std::vector<Vector3>& forces);

private:
    // Corotational FEM
    void computeDeformationGradients();
    void computeElasticForces();

    // Lame parameters
    float mu, lambda; // Shear modulus, bulk modulus
};

// Shape Matching for Fast Deformation
class ShapeMatching {
public:
    std::vector<Vector3> restPositions;    // Reference shape
    std::vector<float> particleMasses;
    Vector3 centerOfMass;

    void initialize(const std::vector<Vector3>& positions, const std::vector<float>& masses);

    // Cluster-based shape matching
    void applyShapeMatching(std::vector<Vector3>& positions, float dt);

    // Multi-level shape matching
    void multiLevelShapeMatching(std::vector<Vector3>& positions, float dt, int levels);

private:
    // Compute cluster centers and rotations
    void computeClusters(const std::vector<Vector3>& positions,
                        std::vector<Vector3>& clusterPositions,
                        std::vector<Quaternion>& clusterRotations) const;

    // Shape constraints
    float stiffness = 0.5f; // 0 = no matching, 1 = perfect matching
};

// Soft-Rigid Body Coupling
class SoftRigidCoupling {
public:
    struct AttachmentPoint {
        int softBodyParticle;
        int rigidBodyIndex;
        Vector3 rigidLocalPoint;
        float stiffness;
        float damping;
    };

    std::vector<AttachmentPoint> attachmentPoints;

    void coupleBodies(DeformableBody& softBody, std::vector<RigidBody>& rigidBodies, float dt);

    // Two-way coupling (force feedback to rigid bodies)
    void applySoftToRigidForces(DeformableBody& softBody, std::vector<RigidBody>& rigidBodies);

    void applyRigidToSoftConstraints(DeformableBody& softBody, std::vector<RigidBody>& rigidBodies);
};

// Advanced Cloth & Hair with Cosserat Rods
class ClothSystem {
public:
    struct ClothParticle {
        Vector3 position;
        Vector3 velocity;
        Vector3 force;
        float mass;
        bool isFixed;
    };

    struct ClothConstraint {
        int particleA, particleB;
        float restLength;
        float stiffness;
        float damping;
    };

    std::vector<ClothParticle> particles;
    std::vector<ClothConstraint> constraints;

    // Level-of-detail cloth
    enum class LODLevel { HIGH, MEDIUM, LOW };
    LODLevel lodLevel = LODLevel::HIGH;

    void initializeCloth(const Vector3& corner, float width, float height, int resolutionX, int resolutionY);

    void simulate(float dt);

    // Collision with rigid bodies
    void handleClothCollisions(const std::vector<RigidBody*>& obstacles);

    // Self-collision detection and response
    void handleSelfCollisions();

    // Wind forces
    void applyWind(const Vector3& windDirection, float windStrength);

    // LOD management
    void updateLOD(float cameraDistance);

private:
    // XPBD solver for cloth
    void solveConstraintsXPBD(float dt);

    // GPU parallel constraint solving (framework)
    void parallelSolveConstraints();
};

// Hair simulation with Cosserat rods
class CosseratRods {
public:
    struct RodSegment {
        Vector3 position;
        Quaternion orientation;
        Vector3 angularVelocity;
        Vector3 force;
        Vector3 torque;
    };

    struct RodMaterial {
        float bendingModulus = 1e-6f;
        float twistingModulus = 1e-8f;
        float stretchingModulus = 1e6f;
        float density = 1.3e3f; // Hair density kg/m³
    };

    std::vector<RodSegment> segments;
    RodMaterial material;
    std::vector<Vector3> rootPositions; // Fixed root positions

    void initializeHairStrands(int strandCount, int segmentsPerStrand, float strandLength);

    void simulate(float dt);

    // Collision with scalp
    void handleScalpCollisions();

    // Wind and airflow forces
    void applyAirflow(const Vector3& airflowVelocity, float density);
};

// Vehicle & Flight Physics
class VehiclePhysics {
public:
    struct Wheel {
        Vector3 position;
        Vector3 rotationAxis;
        float radius;
        float width;
        float suspensionLength;
        float suspensionStiffness;
        float suspensionDamping;
    };

    struct TireModel {
        float lateralStiffness = 20.0f;   // Cornering stiffness
        float longitudinalStiffness = 100.0f;
        float rollingResistance = 0.01f;
        float staticFrictionCoeff = 0.9f;
        float kineticFrictionCoeff = 0.7f;

        Vector3 getLateralForce(float slipAngle, float normalForce) const;
        Vector3 getLongitudinalForce(float slipRatio, float normalForce) const;
    };

    RigidBody* chassis;
    std::vector<Wheel> wheels;
    TireModel tireModel;

    // Aerodynamics
    float dragCoefficient = 0.3f;
    float frontalArea = 2.0f;
    float airDensity = 1.225f;

    void initializeVehicle(float mass, const Vector3& dimensions);

    void updatePhysics(float dt, const Vector3& inputForces);

    // Tire-ground contact and friction
    void updateTireForces(float dt);

    // Suspension forces
    Vector3 computeSuspensionForce(const Wheel& wheel, float compression) const;

    // Aerodynamic forces
    Vector3 computeAerodynamicForce(const Vector3& velocity) const;
};

// Particle Systems with GPU acceleration and fluid coupling
class AdvancedParticleSystem {
public:
    struct Particle {
        Vector3 position;
        Vector3 velocity;
        Vector3 acceleration;
        float mass;
        float life;
        float size;
        Vector3 color;
        bool active;
    };

    std::vector<Particle> particles;
    std::vector<Vector3> forces;
    Vector3 gravity = Vector3(0, -9.81f, 0);

    // Modular emitters
    struct ParticleEmitter {
        Vector3 position;
        Vector3 direction;
        float spreadAngle;
        float emissionRate;
        float particleLife;
        Vector3 initialVelocity;
        float initialSize;

        void emitParticles(std::vector<Particle>& particles, float dt);
    };

    std::vector<ParticleEmitter> emitters;

    // GPU compute shader simulation (framework)
    void simulateGPU(float dt);

    // CPU fallback
    void simulateCPU(float dt);

    // Particle-fluid coupling
    void coupleWithFluid(const std::vector<Vector3>& fluidVelocities,
                        const std::vector<float>& fluidDensities);

    // Volumetric rendering
    std::vector<Vector3> renderVolumetric(const std::vector<std::vector<Vector3>>& frameBuffer);

    // Advanced emission properties
    struct EmissionProperties {
        float lifetime = 1.0f;
        Vector3 colorOverLife[3]; // Start, mid, end
        float sizeOverLife[3];
        std::function<Vector3(float)> customForce; // Lambda for custom forces
    };

    EmissionProperties properties;

    // Sorting and rendering
    void sortParticles(const Vector3& cameraPosition);
};

// Physics World orchestrator
class PhysicsWorld : public System {
public:
    std::vector<RigidBody> rigidBodies;
    std::vector<std::shared_ptr<Constraint>> globalConstraints;

    CCD ccd;
    FractureSystem fractureSystem;
    ArticulatedBody articulatedBody;
    DeformableBody deformableBody;
    SoftRigidCoupling coupling;
    ClothSystem clothSystem;
    CosseratRods hairSystem;
    VehiclePhysics vehicleSystems;
    AdvancedParticleSystem particleSystem;

    // Simulation parameters
    Vector3 gravity = Vector3(0, -9.81f, 0);
    float fixedTimeStep = 1.0f / 60.0f;
    int maxSubSteps = 10;
    float baumgarteFactor = 0.1f; // Constraint stabilization

    void initialize();

    void step(float dt);

    // Broad phase collision detection
    std::vector<std::pair<RigidBody*, RigidBody*>> broadPhase();

    // Narrow phase
    std::vector<CCD::CollisionData> narrowPhase(const std::vector<std::pair<RigidBody*, RigidBody*>>& pairs);

    // Resolve collisions
    void resolveCollisions(const std::vector<CCD::CollisionData>& collisions, float dt);

    // Constraint solver
    void solveConstraints(float dt);

    // Island-based solving for performance
    void solveIslands(const std::vector<RigidBody*>& bodies,
                     const std::vector<std::shared_ptr<Constraint>>& constraints,
                     const std::vector<CCD::CollisionData>& collisions);

private:
    // Constraint islands
    std::vector<std::vector<RigidBody*>> islands;
    std::vector<std::vector<std::shared_ptr<Constraint>>> islandConstraints;
};

} // namespace NeutralGameEngine

#endif // NEUTRAL_GAMEENGINE_PHYSICS_SYSTEM_H
