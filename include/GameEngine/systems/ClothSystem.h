#ifndef FOUNDRY_GAMEENGINE_CLOTH_SYSTEM_H
#define FOUNDRY_GAMEENGINE_CLOTH_SYSTEM_H

#include <vector>
#include <unordered_map>
#include "../../core/System.h"
#include "../../math/Vector3.h"
#include "../../math/Quaternion.h"

namespace FoundryEngine {

class ClothSystem : public System {
public:
    enum class MaterialType {
        SILK,       // High elasticity, low mass
        COTTON,     // Medium elasticity, medium friction
        LEATHER,    // Low elasticity, high friction
        RUBBER      // Very high elasticity, low friction
    };

    struct Particle {
        Vector3 position;
        Vector3 oldPosition;
        Vector3 velocity;
        Vector3 forceAccum;
        float mass = 1.0f;
        bool fixed = false;
        float frictionCoeff = 0.1f;
        float dampeningCoeff = 0.99f;

        Particle(const Vector3& pos, float m = 1.0f) : position(pos), oldPosition(pos), mass(m),
            forceAccum(0,0,0), velocity(0,0,0) {}

        void applyForce(const Vector3& force) { forceAccum += force; }
        void clearForces() { forceAccum = Vector3(0,0,0); }
        Vector3 getAcceleration() const { return forceAccum / mass; }

        void integrate(float dt) {
            if (fixed) return;

            // Verlet integration with velocity
            Vector3 accel = getAcceleration();
            Vector3 newPos = position + (position - oldPosition) * dampeningCoeff + accel * dt * dt;

            // Update velocity
            velocity = (newPos - position) / dt;

            oldPosition = position;
            position = newPos;

            clearForces();
        }
    };

    struct Spring {
        int p1, p2;
        float restLength;
        float stiffness = 1000.0f;  // Advanced stiffness
        float damping = 0.99f;
        float strength = 1.0f;     // For tearing
        bool active = true;

        // Spring types
        enum Type { STRUCTURAL, SHEAR, BEND };
        Type type = STRUCTURAL;
    };

    struct CollisionPlane {
        Vector3 normal;
        float distance;
        float friction = 1.0f;
    };

    struct WindField {
        Vector3 direction;
        float strength = 1.0f;
        bool turbulent = true;
    };

    MaterialType material_ = MaterialType::COTTON;
    std::vector<Particle> particles_;
    std::vector<Spring> springs_;
    std::vector<CollisionPlane> collisionPlanes_;
    WindField windField_;
    float dt_ = 0.016f;
    Vector3 gravity_ = Vector3(0, -9.81f, 0);
    bool tearingEnabled_ = false;
    float tearThreshold_ = 2.0f;
    int constraintIterations_ = 15;
    bool selfCollision_ = true;
    float particleRadius_ = 0.01f;
    std::unordered_map<int, std::vector<int>> spatialGrid_;

    void setMaterial(MaterialType mat) {
        material_ = mat;
        switch (mat) {
            case MaterialType::SILK:
                for (auto& s : springs_) {
                    s.stiffness = 800.0f;
                    s.damping = 0.995f;
                }
                for (auto& p : particles_) {
                    p.mass = 0.8f;
                    p.frictionCoeff = 0.05f;
                }
                break;
            case MaterialType::COTTON:
                for (auto& s : springs_) {
                    s.stiffness = 1000.0f;
                    s.damping = 0.99f;
                }
                for (auto& p : particles_) {
                    p.mass = 1.0f;
                    p.frictionCoeff = 0.1f;
                }
                break;
            case MaterialType::LEATHER:
                for (auto& s : springs_) {
                    s.stiffness = 1500.0f;
                    s.damping = 0.98f;
                }
                for (auto& p : particles_) {
                    p.mass = 1.2f;
                    p.frictionCoeff = 0.3f;
                }
                break;
            case MaterialType::RUBBER:
                for (auto& s : springs_) {
                    s.stiffness = 3000.0f;
                    s.damping = 0.999f;
                }
                for (auto& p : particles_) {
                    p.mass = 0.9f;
                    p.frictionCoeff = 0.02f;
                }
                break;
            default:
                // Unknown material type; consider logging or asserting here
                break;
        }
    }

    // Create advanced cloth grid
    void createClothGrid(int width, int height, float spacing, const Vector3& origin) {
        particles_.clear();
        springs_.clear();

        // Create particles with initial masses and properties
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                Vector3 pos = origin + Vector3(w * spacing, 0, h * spacing);
                particles_.emplace_back(pos);
                // Pin corners and top edge periodically
                if (h == 0 && (w == 0 || w == width-1 || (w % 3 == 0))) {
                    particles_.back().fixed = true;
                }
            }
        }

        setMaterial(material_);

        // Add multiple types of springs
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                int idx = h * width + w;

                // Structural springs
                if (w < width - 1) {
                    springs_.push_back({idx, idx + 1, spacing, 1000.0f, 0.99f, 1.0f, true, Spring::STRUCTURAL});
                }
                if (h < height - 1) {
                    springs_.push_back({idx, idx + width, spacing, 1000.0f, 0.99f, 1.0f, true, Spring::STRUCTURAL});
                }

                // Shear springs (diagonals)
                if (w < width - 1 && h < height - 1) {
                    springs_.push_back({idx, idx + width + 1, spacing * 1.414f, 500.0f, 0.99f, 1.0f, true, Spring::SHEAR});
                    springs_.push_back({idx + 1, idx + width, spacing * 1.414f, 500.0f, 0.99f, 1.0f, true, Spring::SHEAR});
                }

                // Bending springs (every other particle)
                if (w < width - 2) {
                    springs_.push_back({idx, idx + 2, spacing * 2.0f, 100.0f, 0.99f, 1.0f, true, Spring::BEND});
                }
                if (h < height - 2) {
                    springs_.push_back({idx, idx + 2 * width, spacing * 2.0f, 100.0f, 0.99f, 1.0f, true, Spring::BEND});
                }

                // Cross-diagonal bending
                if (w < width - 2 && h < height - 2) {
                    springs_.push_back({idx, idx + 2 * width + 2, spacing * 2.828f, 50.0f, 0.99f, 1.0f, true, Spring::BEND});
                    springs_.push_back({idx + 2, idx + 2 * width, spacing * 2.828f, 50.0f, 0.99f, 1.0f, true, Spring::BEND});
                }
            }
        }
    }

    void applyForces(float dt) {
        for (auto& p : particles_) {
            if (!p.fixed) {
                // Gravity
                p.applyForce(gravity_ * p.mass);

                // Wind force (turbulent)
                if (windField_.turbulent) {
                    float turbulence = (rand() % 1000 - 500) / 500.0f; // Random -1 to 1
                    Vector3 wind = windField_.direction * windField_.strength * (1 + turbulence * 0.3f);
                    p.applyForce(wind * p.mass);
                } else {
                    p.applyForce(windField_.direction * windField_.strength * p.mass);
                }

                // Air resistance
                p.applyForce(-p.velocity * p.velocity.magnitude() * 0.1f);
            }
        }
    }

    void satisfySpringConstraints() {
        for (int iter = 0; iter < constraintIterations_; ++iter) {
            for (auto& s : springs_) {
                if (!s.active) continue;

                Particle& p1 = particles_[s.p1];
                Particle& p2 = particles_[s.p2];

                Vector3 delta = p2.position - p1.position;
                float currentLength = delta.magnitude();

                if (currentLength <= 0) continue;

                float diff = (currentLength - s.restLength) / currentLength;

                // Tearing mechanics
                if (tearingEnabled_ && diff > tearThreshold_) {
                    s.active = false;
                    continue;
                }

                float correctionMagnitude = diff * s.stiffness * s.strength;
                Vector3 correction = delta * (correctionMagnitude * 0.5f);

                if (!p1.fixed) p1.position += correction;
                if (!p2.fixed) p2.position -= correction;
            }
        }
    }

    void handleCollisions() {
        // Collision with planes
        for (auto& plane : collisionPlanes_) {
            for (auto& p : particles_) {
                if (p.fixed) continue;

                float dist = plane.normal.dot(p.position) - plane.distance;
                if (dist < particleRadius_) {
                    p.position -= plane.normal * (dist - particleRadius_);

                    // Friction
                    Vector3 vel = p.velocity;
                    float dot = vel.dot(plane.normal);
                    Vector3 frictionVel = vel - plane.normal * dot;
                    frictionVel *= (1.0f - plane.friction);
                    vel = plane.normal * dot + frictionVel;
                    p.velocity = vel;
                }
            }
        }

        // Self-collisions (advanced, uses spatial partitioning)
        if (selfCollision_) {
            updateSpatialGrid();
            resolveSelfCollisions();
        }
    }

    void updateSpatialGrid() {
        spatialGrid_.clear();
        float cellSize = particleRadius_ * 2.5f;

        for (size_t i = 0; i < particles_.size(); ++i) {
            int cellX = static_cast<int>(particles_[i].position.x / cellSize);
            int cellY = static_cast<int>(particles_[i].position.y / cellSize);
            int cellZ = static_cast<int>(particles_[i].position.z / cellSize);
            int cellKey = cellX * 73856093 ^ cellY * 19349663 ^ cellZ * 83492791;

            spatialGrid_[cellKey].push_back(i);
        }
    }

    void resolveSelfCollisions() {
        for (auto& cell : spatialGrid_) {
            auto& indices = cell.second;
            for (size_t i = 0; i < indices.size(); ++i) {
                for (size_t j = i + 1; j < indices.size(); ++j) {
                    int id1 = indices[i];
                    int id2 = indices[j];

                    Particle& p1 = particles_[id1];
                    Particle& p2 = particles_[id2];

                    if (p1.fixed && p2.fixed) continue;

                    Vector3 delta = p2.position - p1.position;
                    float dist = delta.magnitude();
                    float minDist = particleRadius_ * 2.0f;

                    if (dist < minDist && dist > 0) {
                        Vector3 norm = delta / dist;
                        float correction = (minDist - dist) * 0.5f;
                        Vector3 corrVec = norm * correction;

                        if (!p1.fixed) p1.position -= corrVec;
                        if (!p2.fixed) p2.position += corrVec;

                        // Dampen velocities
                        Vector3 relVel = p2.velocity - p1.velocity;
                        float velAlongNorm = relVel.dot(norm);
                        if (velAlongNorm > 0) {
                            Vector3 impulse = norm * (velAlongNorm * 0.5f);
                            if (!p1.fixed) p1.velocity += impulse;
                            if (!p2.fixed) p2.velocity -= impulse;
                        }
                    }
                }
            }
        }
    }

    void update(float deltaTime) override {
        dt_ = deltaTime;
        timeAccumulator_ += deltaTime;

        // Fixed timestep for stability
        float fixedDt = 0.016f;
        while (timeAccumulator_ >= fixedDt) {
            applyForces(fixedDt);
            satisfySpringConstraints();
            handleCollisions();

            for (auto& p : particles_) {
                p.integrate(fixedDt);
            }

            timeAccumulator_ -= fixedDt;
        }
    }

    // Advanced rendering data with normals and texcoords
    struct RenderData {
        std::vector<Vector3> positions;
        std::vector<Vector3> normals;
        std::vector<float> texcoords;
        std::vector<int> indices;
    };

    RenderData getRenderData() {
        RenderData data;
        int width = calculateWidth();
        int height = particles_.size() / width;

        // Generate vertices with normals
        data.positions.reserve(particles_.size());
        data.normals.reserve(particles_.size());
        data.texcoords.reserve(particles_.size() * 2);

        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                int idx = h * width + w;
                data.positions.push_back(particles_[idx].position);

                // Calculate normal (cross product of adjacent triangle)
                Vector3 normal = calculateNormal(w, h, width, height);
                data.normals.push_back(normal);

                // Texture coordinates
                data.texcoords.push_back(w / float(width));
                data.texcoords.push_back(h / float(height));
            }
        }

        // Generate indices (two triangles per quad)
        data.indices.reserve((width - 1) * (height - 1) * 6);
        for (int h = 0; h < height - 1; ++h) {
            for (int w = 0; w < width - 1; ++w) {
                int idx = h * width + w;
                data.indices.insert(data.indices.end(), {idx, idx + 1, idx + width});
                data.indices.insert(data.indices.end(), {idx + 1, idx + width + 1, idx + width});
            }
        }

        return data;
    }

    void addCollisionPlane(const Vector3& normal, float distance, float friction = 1.0f) {
        collisionPlanes_.push_back({normal.normalized(), distance, friction});
    }

    void setWind(const Vector3& direction, float strength, bool turbulent = true) {
        windField_ = {direction.normalized(), strength, turbulent};
    }

    void enableTearing(bool enable, float threshold = 2.0f) {
        tearingEnabled_ = enable;
        tearThreshold_ = threshold;
    }

private:
    float timeAccumulator_ = 0.0f;

    int calculateWidth() const {
        int n = particles_.size();
        int w = 1;
        while (w * w < n) ++w;
        return w;
    }

    Vector3 calculateNormal(int w, int h, int width, int height) const {
        Vector3 normal(0, 0, 0);
        int idx = h * width + w;

        // Cross product of adjacent triangles
        if (w < width - 1 && h < height - 1) {
            Vector3 p0 = particles_[idx].position;
            Vector3 p1 = particles_[idx + 1].position;
            Vector3 p2 = particles_[idx + width].position;

            Vector3 u = p1 - p0;
            Vector3 v = p2 - p0;
            normal = u.cross(v);
        }

        return normal.normalized();
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_CLOTH_SYSTEM_H
