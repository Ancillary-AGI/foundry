#ifndef NEUTRAL_GAMEENGINE_FLUID_SYSTEM_H
#define NEUTRAL_GAMEENGINE_FLUID_SYSTEM_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include "../../core/System.h"
#include "../../math/Vector3.h"
#include "../../optimization/SpatialPartition.h"

namespace FoundryEngine {

class FluidSystem : public System {
public:
    enum class FluidType {
        WATER,
        OIL,
        MERCURY,
        GAS,
        CUSTOM
    };

    enum class SolverType {
        JACOBI,
        GAUSS_SEIDEL,
        CONJUGATE_GRADIENT,
        MULTIGRID
    };

    // Advanced particle for PCISPH/WCSPH/SPH fluid
    struct Particle {
        Vector3 position;
        Vector3 velocity;
        Vector3 acceleration;
        Vector3 oldPosition;
        Vector3 forceAccum;
        float density;
        float pressure;
        float mass;
        float temperature;
        float viscosity;
        int fluidTypeId;
        bool isBoundary = false;

        // Advanced properties
        float surfaceTension;
        float surfaceNormCoeff;  // For surface tension
        Vector3 surfaceNormal;
        float curlMagnitude;     // For vorticity confinement

        Particle(const Vector3& pos, float m = 1.0f, int type = 0)
            : position(pos), oldPosition(pos), mass(m), fluidTypeId(type),
              density(0), pressure(0), temperature(293.15f), viscosity(0.001f),
              surfaceTension(0.072f), surfaceNormCoeff(0), curlMagnitude(0) {}

        void resetForces() { forceAccum = Vector3(0,0,0); acceleration = Vector3(0,0,0); }
        void applyForce(const Vector3& f) { forceAccum += f; }
        void integrate(float dt) {
            velocity += acceleration * dt;
            position += velocity * dt;
        }

        void verletIntegrate(float dt) {
            Vector3 temp = position;
            position = position + (position - oldPosition) + acceleration * dt * dt;
            oldPosition = temp;
            velocity = (position - oldPosition) / dt;
        }
    };

    struct FluidProperties {
        FluidType type;
        float restDensity;
        float surfaceTension;
        float viscosity;
        float gasStiffness;
        float bulkModulus;
        float thermalConductivity;
    };

    struct NeighborCache {
        std::vector<int> neighbors;
        float maxDistance;
        void rebuild() { neighbors.clear(); }
    };

    std::vector<Particle> particles_;
    std::vector<NeighborCache> neighborCache_;
    std::unordered_map<int, FluidProperties> fluidProperties_;
    SpatialPartition spatialPartition_;

    // Advanced parameters
    float smoothingLength_ = 0.15f;
    float timeStep_ = 0.016f;
    float restDensity_ = 1000.0f;
    float gasConstant_ = 2000.0f;
    float viscosityCoefficient_ = 0.01f;
    float surfaceTensionCoeff_ = 0.5f;
    Vector3 gravity_ = Vector3(0, -9.81f, 0);
    bool enableViscosity_ = true;
    bool enableSurfaceTension_ = true;
    bool enableTemperature_ = false;
    bool usePCISPH_ = true;  // Position-based correction SPH

    SolverType pressureSolverType_ = SolverType::JACOBI;
    int pressureIterations_ = 100;
    float pressureTolerance_ = 1e-6f;

    // Boundary conditions
    struct Boundary {
        enum Type { BOX, SPHERE, PLANE, MESH };
        Type type;
        Vector3 center;
        float radius;
        Vector3 normal;
        float distance;
    };
    std::vector<Boundary> boundaries_;

    // Initialize fluid properties
    void defineFluidProperties() {
        fluidProperties_[0] = {FluidType::WATER, 1000.0f, 0.072f, 0.001f, 2000.0f, 2.2e9f, 0.58f};
        fluidProperties_[1] = {FluidType::OIL, 900.0f, 0.027f, 0.065f, 1500.0f, 1.5e9f, 0.19f};
        fluidProperties_[2] = {FluidType::MERCURY, 13534.0f, 0.47f, 0.0015f, 28400.0f, 2.5e10f, 8.34f};
        fluidProperties_[3] = {FluidType::GAS, 1.225f, 0.0f, 1e-5f, 1.4e5f, 1.4e5f, 0.025f};
    }

    // Add particle with specific fluid type
    void addParticle(const Vector3& position, const Vector3& velocity = Vector3(0,0,0),
                    int fluidTypeId = 0, float mass = 1.0f) {
        particles_.emplace_back(position, mass, fluidTypeId);
        particles_.back().velocity = velocity;
        particles_.back().density = fluidProperties_[fluidTypeId].restDensity;
        neighborCache_.resize(particles_.size());
    }

    // Multiphase initialization
    void initializeMultiphaseSphere(const Vector3& center, float radius, int innerType, int outerType,
                                   float innerRadius, int numParticles = 1000) {
        // Create spherical fluid region
        for (int i = 0; i < numParticles; ++i) {
            Vector3 pos;
            do {
                pos.x = (rand() / float(RAND_MAX) - 0.5f) * 2 * radius;
                pos.y = (rand() / float(RAND_MAX) - 0.5f) * 2 * radius;
                pos.z = (rand() / float(RAND_MAX) - 0.5f) * 2 * radius;
            } while (pos.magnitudeSq() > radius * radius);

            pos += center;
            float distFromCenter = (pos - center).magnitude();
            int fluidId = (distFromCenter < innerRadius) ? innerType : outerType;

            addParticle(pos, Vector3(0,0,0), fluidId);
        }
    }

    // Find neighbors using spatial hashing
    void findNeighbors(int particleIndex) {
        neighborCache_[particleIndex].rebuild();
        float searchRadius = smoothingLength_ * 2.5f;

        spatialPartition_.queryNeighbors(particles_[particleIndex].position, searchRadius,
                                        neighborCache_[particleIndex].neighbors);

        // Filter by actual distance
        auto& neighbors = neighborCache_[particleIndex].neighbors;
        neighbors.erase(std::remove_if(neighbors.begin(), neighbors.end(),
                        [&](int j) {
                            if (j == particleIndex) return true;
                            return (particles_[j].position - particles_[particleIndex].position).magnitudeSq() >
                                   smoothingLength_ * smoothingLength_;
                        }), neighbors.end());
    }

    // Advanced kernels collection
    class SmoothingKernels {
    public:
        static float poly6(float r, float h) {
            if (r >= h) return 0.0f;
            float h3 = h * h * h;
            float h9 = h3 * h3 * h3;
            float r6 = r * r * r * r * r * r;
            return 315.0f / (64.0f * M_PI * h9) * (h*h - r*r) * (h*h - r*r) * (h*h - r*r);
        }

        static Vector3 gradSpiky(const Vector3& r, float dist, float h) {
            if (dist >= h || dist <= 0) return Vector3(0,0,0);
            float h6 = h * h * h * h * h * h;
            return -45.0f / (M_PI * h6) * (h - dist) * (h - dist) / dist * r;
        }

        static float laplacianViscosity(float r, float h) {
            if (r >= h) return 0.0f;
            float h6 = h * h * h * h * h * h;
            return 45.0f / (M_PI * h6) * (h - r);
        }

        static Vector3 gradPoly6(const Vector3& r, float dist, float h) {
            if (dist >= h) return Vector3(0,0,0);
            float h3 = h * h * h;
            float h9 = h3 * h3 * h3;
            float r5 = dist * dist * dist * dist * dist;
            return -945.0f / (32.0f * M_PI * h9) * (h*h - dist*dist) * (h*h - dist*dist) / r * r;
        }

        // Anisotropic kernels for surface tension - simplified
        static Vector3 anisotropicTensor(const Vector3& r, float dist, float h) {
            if (dist >= h) return Vector3(0,0,0);

            Vector3 dir = r / dist;
            float w = 315.0f / (64.0f * M_PI * h*h*h*h*h*h*h*h*h) *
                     (h*h - dist*dist) * (h*h - dist*dist) * (h*h - dist*dist);

            return dir * (dir.dot(dir) * w);
        }
    };

    // Density computation with multiphase
    void computeDensityPressure() {
        const float h = smoothingLength_;

        for (size_t i = 0; i < particles_.size(); ++i) {
            if (particles_[i].isBoundary) continue;

            float density = 0.0f;
            const auto& neighbors = neighborCache_[i].neighbors;

            for (int j : neighbors) {
                Vector3 diff = particles_[j].position - particles_[i].position;
                float dist = diff.magnitude();
                density += particles_[j].mass * SmoothingKernels::poly6(dist, h);
            }

            particles_[i].density = density;

            // Multiphase pressure
            auto& props = fluidProperties_[particles_[i].fluidTypeId];
            if (particles_[i].density > 0) {
                particles_[i].pressure = props.gasStiffness *
                                        (std::pow(particles_[i].density / props.restDensity, 7.0f) - 1.0f);
            } else {
                particles_[i].pressure = 0.0f;
            }
        }
    }

    // Surface properties computation
    void computeSurfaceProperties() {
        const float h = smoothingLength_;

        for (size_t i = 0; i < particles_.size(); ++i) {
            if (particles_[i].isBoundary) continue;

            Vector3 surfaceNormal(0,0,0);
            float surfaceTension = 0.0f;
            const auto& neighbors = neighborCache_[i].neighbors;

            for (int j : neighbors) {
                Vector3 diff = particles_[j].position - particles_[i].position;
                float dist = diff.magnitude();
                if (dist > 0) {
                    Vector3 grad = SmoothingKernels::gradPoly6(diff, dist, h);
                    surfaceNormal += grad * particles_[j].mass / particles_[j].density;
                }
            }

            particles_[i].surfaceNormal = surfaceNormal;
            particles_[i].surfaceNormCoeff = surfaceNormal.magnitude();

            // Surface tension color field
            if (particles_[i].surfaceNormCoeff > 0.1f) {
                for (int j : neighbors) {
                    Vector3 diff = particles_[j].position - particles_[i].position;
                    float dist = diff.magnitude();
                    surfaceTension += particles_[j].mass *
                                    (1.0f / particles_[j].density) *
                                    SmoothingKernels::laplacianViscosity(dist, h);
                }
                particles_[i].surfaceTension = -surfaceTensionCoeff_ * particles_[i].mass * surfaceTension;
            }
        }
    }

    // Vorticity confinement for realistic fluid behavior
    void computeVorticityConfinement() {
        const float h = smoothingLength_;

        for (size_t i = 0; i < particles_.size(); ++i) {
            if (particles_[i].isBoundary) continue;

            Vector3 curl(0,0,0);
            const auto& neighbors = neighborCache_[i].neighbors;

            for (int j : neighbors) {
                Vector3 v_diff = particles_[j].velocity - particles_[i].velocity;
                Vector3 r_diff = particles_[j].position - particles_[i].position;
                float dist = r_diff.magnitude();

                if (dist > 0) {
                    Vector3 grad = SmoothingKernels::gradPoly6(r_diff, dist, h);
                    curl += v_diff.cross(grad) * (particles_[j].mass / particles_[j].density);
                }
            }

            particles_[i].curlMagnitude = curl.magnitude();
        }
    }

    // Advanced force computation
    void computeForces() {
        const float h = smoothingLength_;

        for (auto& p : particles_) {
            p.resetForces();

            if (!p.isBoundary) {
                p.applyForce(gravity_ * p.mass);
            }
        }

        for (size_t i = 0; i < particles_.size(); ++i) {
            if (particles_[i].isBoundary) continue;

            Vector3 pressureForce(0,0,0);
            Vector3 viscosityForce(0,0,0);
            Vector3 surfaceForce(0,0,0);
            Vector3 vorticityForce(0,0,0);

            const auto& neighbors = neighborCache_[i].neighbors;

            for (int j : neighbors) {
                Vector3 r_ij = particles_[j].position - particles_[i].position;
                float dist = r_ij.magnitude();

                if (dist <= 0) continue;

                Vector3 gradKernel = SmoothingKernels::gradSpiky(r_ij, dist, h);
                Vector3 unitDir = r_ij / dist;

                // Pressure force
                float pressureTerm = (particles_[i].pressure / (particles_[i].density * particles_[i].density) +
                                    particles_[j].pressure / (particles_[j].density * particles_[j].density));
                pressureForce += -particles_[j].mass * pressureTerm * gradKernel;

                // Viscosity force
                if (enableViscosity_) {
                    viscosityForce += viscosityCoefficient_ * particles_[j].mass *
                                    (particles_[j].velocity - particles_[i].velocity) /
                                    particles_[j].density * SmoothingKernels::laplacianViscosity(dist, h);
                }

                // Surface tension force
                if (enableSurfaceTension_ && particles_[i].surfaceNormCoeff > 0.1f &&
                    particles_[j].surfaceNormCoeff > 0.1f) {
                    Vector3 n_i = particles_[i].surfaceNormal / particles_[i].surfaceNormCoeff;
                    Vector3 n_j = particles_[j].surfaceNormal / particles_[j].surfaceNormCoeff;

                    // Cohesion term
                    surfaceForce += surfaceTensionCoeff_ * particles_[j].mass * (n_i - n_j);

                    // Curvature term
                    float C_i = particles_[i].surfaceNormCoeff;
                    float C_j = particles_[j].surfaceNormCoeff;
                    surfaceForce += -particles_[j].mass * (C_i + C_j) * gradKernel / 2.0f;
                }
            }

            // Vorticity confinement
            if (particles_[i].curlMagnitude > 0.001f) {
                Vector3 eta = particles_[i].surfaceNormal / particles_[i].surfaceNormCoeff;
                vorticityForce = eta.cross(curl) * 0.1f; // Vorticity confinement coefficient
            }

            particles_[i].forceAccum += pressureForce + viscosityForce + surfaceForce + vorticityForce;
        }

        // Temperature effects (optional)
        if (enableTemperature_) {
            for (auto& p : particles_) {
                float tempDiff = 293.15f - p.temperature; // Ambient temperature
                p.temperature += tempDiff * 0.1f; // Heat exchange

                // Thermal buoyancy
                p.applyForce(Vector3(0, tempDiff * 0.001f, 0));
            }
        }
    }

    // Boundary handling
    void handleBoundaries() {
        for (auto& boundary : boundaries_) {
            switch (boundary.type) {
                case Boundary::BOX: {
                    for (auto& p : particles_) {
                        // Handle box boundaries with reflection
                        if (p.position.x < boundary.center.x - boundary.radius) {
                            p.position.x = 2 * (boundary.center.x - boundary.radius) - p.position.x;
                            p.velocity.x *= -0.8f;
                        } else if (p.position.x > boundary.center.x + boundary.radius) {
                            p.position.x = 2 * (boundary.center.x + boundary.radius) - p.position.x;
                            p.velocity.x *= -0.8f;
                        }
                        // Y and Z boundaries...
                    }
                    break;
                }
                case Boundary::PLANE: {
                    for (auto& p : particles_) {
                        float dist = boundary.normal.dot(p.position) - boundary.distance;
                        if (dist < 0) {
                            p.position -= boundary.normal * dist;
                            Vector3 reflection = p.velocity -
                                               2 * boundary.normal * (boundary.normal.dot(p.velocity));
                            p.velocity = reflection * 0.8f;
                        }
                    }
                    break;
                }
                default:
                    // Unknown boundary type; consider logging or asserting here
                    break;
            }
        }
    }

    // Fluid-structure interaction
    void computeFluidStructureInteraction(const std::vector<Vector3>& rigidBodyPositions,
                                         const std::vector<Vector3>& rigidBodyNormals) {
        // Coupling between fluid and rigid bodies
        for (size_t i = 0; i < particles_.size(); ++i) {
            for (size_t j = 0; j < rigidBodyPositions.size(); ++j) {
                Vector3 diff = rigidBodyPositions[j] - particles_[i].position;
                float dist = diff.magnitude();

                if (dist < smoothingLength_) {
                    // Apply repulsive force from rigid body
                    Vector3 repulsion = rigidBodyNormals[j] * (1.0f / (1.0f + dist * dist)) * 100.0f;
                    particles_[i].applyForce(repulsion);
                }
            }
        }
    }

    // Main update function
    void update(float deltaTime) override {
        timeAccumulator_ += deltaTime;

        // Fixed timestep for stability
        while (timeAccumulator_ >= timeStep_) {
            // Spatial partitioning update
            spatialPartition_.clear();
            for (size_t i = 0; i < particles_.size(); ++i) {
                spatialPartition_.insert(particles_[i].position, i);
            }

            // Find neighbors for all particles
            for (size_t i = 0; i < particles_.size(); ++i) {
                findNeighbors(i);
            }

            // Compute fluid properties
            computeDensityPressure();
            computeSurfaceProperties();
            computeVorticityConfinement();

            // Compute and apply forces
            computeForces();

            // Handle boundaries
            handleBoundaries();

            // Integrate
            if (usePCISPH_) {
                positionBasedCorrection();
            } else {
                simpleIntegration();
            }

            timeAccumulator_ -= timeStep_;
        }
    }

    // Position-based correction for incompressibility
    void positionBasedCorrection() {
        const float h = smoothingLength_;
        std::vector<Vector3> positionDelta(particles_.size(), Vector3(0,0,0));

        for (size_t i = 0; i < particles_.size(); ++i) {
            if (particles_[i].isBoundary) continue;

            Vector3 delta(0,0,0);
            const auto& neighbors = neighborCache_[i].neighbors;

            for (int j : neighbors) {
                Vector3 r_ij = particles_[j].position - particles_[i].position;
                float dist = r_ij.magnitude();

                if (dist > 0) {
                    Vector3 gradKernel = SmoothingKernels::gradSpiky(r_ij, dist, h);
                    float lambda = -particles_[i].density + fluidProperties_[particles_[i].fluidTypeId].restDensity;

                    if (particles_[j].density > 0) {
                        lambda /= particles_[j].density;
                    }

                    delta += lambda * gradKernel;
                }
            }

            positionDelta[i] = delta;
            particles_[i].position += delta;

            // Re-compute velocity
            if ((particles_[i].position - particles_[i].oldPosition).magnitude() > 0) {
                particles_[i].velocity = (particles_[i].position - particles_[i].oldPosition) / timeStep_;
            }
            particles_[i].oldPosition = particles_[i].position;
        }
    }

    // Simple Euler integration
    void simpleIntegration() {
        for (auto& p : particles_) {
            if (!p.isBoundary) {
                p.acceleration = p.forceAccum / p.mass;
                p.integrate(timeStep_);
                p.oldPosition = p.position;
            }
        }
    }

    // Advanced fluid rendering data
    struct RenderData {
        std::vector<Vector3> positions;
        std::vector<Vector3> velocities;
        std::vector<float> densities;
        std::vector<float> pressures;
        std::vector<int> fluidTypes;
        std::vector<Vector3> surfaceNormals;
    };

    RenderData getRenderData() const {
        RenderData data;
        data.positions.reserve(particles_.size());
        data.velocities.reserve(particles_.size());
        data.densities.reserve(particles_.size());
        data.pressures.reserve(particles_.size());
        data.fluidTypes.reserve(particles_.size());
        data.surfaceNormals.reserve(particles_.size());

        for (const auto& p : particles_) {
            data.positions.push_back(p.position);
            data.velocities.push_back(p.velocity);
            data.densities.push_back(p.density);
            data.pressures.push_back(p.pressure);
            data.fluidTypes.push_back(p.fluidTypeId);
            data.surfaceNormals.push_back(p.surfaceNormal);
        }

        return data;
    }

    void addBoundaryBox(const Vector3& center, float size) {
        boundaries_.push_back({Boundary::BOX, center, size, Vector3(0,0,0), 0});
    }

    void addBoundaryPlane(const Vector3& normal, float distance) {
        boundaries_.push_back({Boundary::PLANE, Vector3(0,0,0), 0, normal.normalized(), distance});
    }

private:
    float timeAccumulator_ = 0.0f;

    // PCISPH intermediate variables
    std::vector<Vector3> predictedPositions_;
    std::vector<float> predictedDensities_;
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_FLUID_SYSTEM_H
