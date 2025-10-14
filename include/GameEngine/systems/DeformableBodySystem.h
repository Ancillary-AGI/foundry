#ifndef FOUNDRY_GAMEENGINE_DEFORMABLE_BODY_SYSTEM_H
#define FOUNDRY_GAMEENGINE_DEFORMABLE_BODY_SYSTEM_H

#include <vector>
#include <map>
#include "../../core/System.h"
#include "../../math/Vector3.h"
#include "../../math/NumericalMethods.h" // For numerical integration if needed

namespace FoundryEngine {

class DeformableBodySystem : public System {
public:
    struct Node {
        Vector3 position;
        Vector3 velocity;
        float mass = 1.0f;
        Vector3 force;

        Node(const Vector3& pos, float m = 1.0f) : position(pos), mass(m) {}
    };

    struct Tetrahedron {
        int n[4]; // Node indices
        float volume;
        float youngsModulus = 1000.0f; // Elasticity
        float poissonRatio = 0.3f;
        Matrix4 deformationGradient; // For FEM

        Tetrahedron(int a, int b, int c, int d) : n{a, b, c, d} {}
    };

    struct Body {
        std::vector<Node> nodes;
        std::vector<Tetrahedron> tets;
        std::vector<int> fixedNodes; // Indices of fixed nodes

        void addTetrahedron(int i0, int i1, int i2, int i3) {
            tets.emplace_back(i0, i1, i2, i3);
        }

        void fixNode(int idx) { fixedNodes.push_back(idx); }
    };

    Body body_;
    float dt_ = 0.01f;
    float damping_ = 0.99f;

    void createFromMesh(const std::vector<Vector3>& vertices, const std::vector<std::vector<int>>& tetConnectivity) {
        body_.nodes.clear();
        for (const auto& v : vertices) {
            body_.nodes.emplace_back(v);
        }
        for (const auto& tet : tetConnectivity) {
            if (tet.size() == 4) {
                body_.addTetrahedron(tet[0], tet[1], tet[2], tet[3]);
            }
        }
        computeRestVolumes();
    }

    void update(float deltaTime) override {
        // Compute internal forces
        computeForces();

        // Integrate (explicit Euler)
        for (size_t i = 0; i < body_.nodes.size(); ++i) {
            if (std::find(body_.fixedNodes.begin(), body_.fixedNodes.end(), i) == body_.fixedNodes.end()) {
                Vector3 accel = body_.nodes[i].force / body_.nodes[i].mass;
                body_.nodes[i].velocity += accel * dt_;
                body_.nodes[i].velocity *= damping_; // Damping
                body_.nodes[i].position += body_.nodes[i].velocity * dt_ + accel * dt_ * dt_ * 0.5f;
            } else {
                body_.nodes[i].velocity = Vector3(0,0,0);
            }
        }
    }

private:
    void computeRestVolumes() {
        for (auto& tet : body_.tets) {
            // Compute rest volume of tetrahedron
            Vector3 p0 = body_.nodes[tet.n[0]].position;
            Vector3 p1 = body_.nodes[tet.n[1]].position - p0;
            Vector3 p2 = body_.nodes[tet.n[2]].position - p0;
            Vector3 p3 = body_.nodes[tet.n[3]].position - p0;
            tet.volume = abs(p1.cross(p2).dot(p3)) / 6.0f;
        }
    }

    void computeForces() {
        // Reset forces to zero, external (gravity)
        for (auto& node : body_.nodes) {
            node.force = Vector3(0, -9.81f * node.mass, 0); // Gravity
        }

        // Add internal elastic forces (simple linear elasticity)
        for (const auto& tet : body_.tets) {
            // Simplified: spring forces (not full FEM)
            Vector3 center(0,0,0);
            for (int i : tet.n) center += body_.nodes[i].position;
            center /= 4;

            for (int i = 0; i < 4; ++i) {
                Vector3 restPos = (body_.nodes[tet.n[(i+1)%4]].position + body_.nodes[tet.n[(i+2)%4]].position + body_.nodes[tet.n[(i+3)%4]].position) / 3;
                Vector3 currentPos = body_.nodes[tet.n[i]].position;
                Vector3 dir = (currentPos - center).normalized();
                Vector3 force = dir * tet.youngsModulus * (currentPos - restPos).magnitude();
                body_.nodes[tet.n[i]].force += force;
            }
        }
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_DEFORMABLE_BODY_SYSTEM_H
