#ifndef FOUNDRY_GAMEENGINE_RASTERIZER_H
#define FOUNDRY_GAMEENGINE_RASTERIZER_H

#include <vector>
#include "../../core/System.h"
#include "../../math/Vector3.h"
#include "../../math/Matrix4.h"

namespace FoundryEngine {

class Rasterizer : public System {
public:
    // Triangle for rendering
    struct Triangle {
        Vector3 vertices[3];
        Vector3 normals[3];
        Vector3 colors[3];

        Triangle() = default;
        Triangle(const std::vector<Vector3>& verts, const std::vector<Vector3>& norms = {}, const std::vector<Vector3>& cols = {}) {
            for (int i = 0; i < 3; ++i) {
                vertices[i] = verts[i];
                normals[i] = norms.empty() ? Vector3(0,1,0) : norms[i];
                colors[i] = cols.empty() ? Vector3(1,1,1) : cols[i];
            }
        }
    };

    std::vector<Triangle> triangles_;

    // MVP matrices
    Matrix4 model_, view_, projection_;
    int viewportWidth_ = 800, viewportHeight_ = 600;

    void setTransformation(const Matrix4& model, const Matrix4& view, const Matrix4& proj) {
        model_ = model;
        view_ = view;
        projection_ = proj;
    }

    void addTriangle(const Triangle& tri) { triangles_.push_back(tri); }

    // Software rasterization to buffer
    void renderToBuffer(std::vector<Vector3>& colorBuffer, std::vector<float>& depthBuffer) {
        colorBuffer.resize(viewportWidth_ * viewportHeight_, Vector3(0,0,0));
        depthBuffer.resize(viewportWidth_ * viewportHeight_, INFINITY);

        for (const auto& tri : triangles_) {
            Triangle transformed = transformTriangle(tri);
            if (isBackfaceCulled(transformed)) continue;
            rasterizeTriangle(transformed, colorBuffer, depthBuffer);
        }
    }

    void update(float deltaTime) override {
        // Update transformations if needed
    }

private:
    Triangle transformTriangle(const Triangle& tri) {
        Triangle transformed;
        Matrix4 mvp = model_ * view_ * projection_;
        for (int i = 0; i < 3; ++i) {
            Vector3 homogeneous = mvp * tri.vertices[i];
            // Perspective divide
            if (homogeneous.z != 0) {
                homogeneous.x /= homogeneous.z;
                homogeneous.y /= homogeneous.z;
            }
            // Viewport transform
            homogeneous.x = (homogeneous.x + 1) * viewportWidth_ / 2;
            homogeneous.y = (1 - homogeneous.y) * viewportHeight_ / 2; // Flip Y
            transformed.vertices[i] = homogeneous;
            transformed.normals[i] = tri.normals[i]; // Transform normals separately
            transformed.colors[i] = tri.colors[i];
        }
        return transformed;
    }

    bool isBackfaceCulled(const Triangle& tri) {
        Vector3 edge1 = tri.vertices[1] - tri.vertices[0];
        Vector3 edge2 = tri.vertices[2] - tri.vertices[0];
        Vector3 normal = edge1.cross(edge2);
        return normal.z < 0; // Cull if facing away from camera
    }

    void rasterizeTriangle(const Triangle& tri, std::vector<Vector3>& colorBuffer, std::vector<float>& depthBuffer) {
        // Find bounding box
        float minX = std::min({tri.vertices[0].x, tri.vertices[1].x, tri.vertices[2].x});
        float maxX = std::max({tri.vertices[0].x, tri.vertices[1].x, tri.vertices[2].x});
        float minY = std::min({tri.vertices[0].y, tri.vertices[1].y, tri.vertices[2].y});
        float maxY = std::max({tri.vertices[0].y, tri.vertices[1].y, tri.vertices[2].y});

        minX = std::max(minX, 0.0f);
        maxX = std::min(maxX, (float)viewportWidth_ - 1);
        minY = std::max(minY, 0.0f);
        maxY = std::min(maxY, (float)viewportHeight_ - 1);

        // Scanline rasterization
        for (int y = (int)minY; y <= (int)maxY; ++y) {
            for (int x = (int)minX; x <= (int)maxX; ++x) {
                BarycentricCoords bc = barycentric(tri, Vector3(x + 0.5f, y + 0.5f, 0));
                if (bc.u >= 0 && bc.v >= 0 && bc.w >= 0) {
                    float interpolatedZ = bc.u * tri.vertices[0].z + bc.v * tri.vertices[1].z + bc.w * tri.vertices[2].z;
                    if (interpolatedZ < depthBuffer[y * viewportWidth_ + x]) {
                        depthBuffer[y * viewportWidth_ + x] = interpolatedZ;
                        // Interpolate other attributes
                        Vector3 color = bc.u * tri.colors[0] + bc.v * tri.colors[1] + bc.w * tri.colors[2];
                        Vector3 normal = bc.u * tri.normals[0] + bc.v * tri.normals[1] + bc.w * tri.normals[2];
                        // Simple shading
                        normal = normal.normalized();
                        Vector3 lightDir = Vector3(1,1,1).normalized();
                        float diffuse = std::max(normal.dot(lightDir), 0.0f);
                        colorBuffer[y * viewportWidth_ + x] = color * (0.1f + diffuse * 0.9f);
                    }
                }
            }
        }
    }

    struct BarycentricCoords {
        float u, v, w;
    };

    BarycentricCoords barycentric(const Triangle& tri, const Vector3& p) {
        Vector3 v0 = tri.vertices[1] - tri.vertices[0];
        Vector3 v1 = tri.vertices[2] - tri.vertices[0];
        Vector3 v2 = p - tri.vertices[0];

        float d00 = v0.dot(v0);
        float d01 = v0.dot(v1);
        float d11 = v1.dot(v1);
        float d20 = v2.dot(v0);
        float d21 = v2.dot(v1);

        float denom = d00 * d11 - d01 * d01;
        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;

        return {u, v, w};
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_RASTERIZER_H
