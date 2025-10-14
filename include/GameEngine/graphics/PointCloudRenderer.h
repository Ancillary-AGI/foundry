#ifndef FOUNDRY_GAMEENGINE_POINT_CLOUD_RENDERER_H
#define FOUNDRY_GAMEENGINE_POINT_CLOUD_RENDERER_H

#include <vector>
#include "../../core/System.h"
#include "../../math/Vector3.h"

namespace FoundryEngine {

class PointCloudRenderer : public System {
public:
    struct Point {
        Vector3 position;
        Vector3 color;
        float radius = 0.01f;
        float intensity = 1.0f;

        Point(const Vector3& pos, const Vector3& col = Vector3(1,1,1), float r = 0.01f, float i = 1.0f)
            : position(pos), color(col), radius(r), intensity(i) {}
    };

    std::vector<Point> points_;

    void addPoint(const Vector3& pos, const Vector3& color = Vector3(1,1,1), float radius = 0.01f) {
        points_.emplace_back(pos, color, radius);
    }

    void loadFromFile(const std::string& filename) {
        // Placeholder: parse PLY, PCD, or XYZ files
        // For now, assume data is added programmatically
    }

    void applyTransformation(const Matrix4& matrix) {
        for (auto& p : points_) {
            p.position = matrix * p.position;
        }
    }

    void filterByDistance(float minDist, float maxDist, const Vector3& referencePoint) {
        std::vector<Point> filtered;
        for (const auto& p : points_) {
            float dist = (p.position - referencePoint).magnitude();
            if (dist >= minDist && dist <= maxDist) {
                filtered.push_back(p);
            }
        }
        points_ = std::move(filtered);
    }

    // Downsampling for performance
    void downsample(float voxelSize) {
        // Simple grid-based downsampling
        std::map<std::tuple<int, int, int>, std::vector<Point>> grid;
        for (const auto& p : points_) {
            auto key = std::make_tuple(
                (int)std::floor(p.position.x / voxelSize),
                (int)std::floor(p.position.y / voxelSize),
                (int)std::floor(p.position.z / voxelSize)
            );
            grid[key].push_back(p);
        }

        points_.clear();
        for (auto& kv : grid) {
            // Average or representative point per voxel
            Vector3 avgPos(0,0,0);
            Vector3 avgColor(0,0,0);
            for (const auto& pt : kv.second) {
                avgPos += pt.position;
                avgColor += pt.color;
            }
            avgPos /= kv.second.size();
            avgColor /= kv.second.size();
            points_.emplace_back(avgPos, avgColor);
        }
    }

    // Depth-based coloring
    void applyDepthColoring(const Vector3& cameraPos) {
        for (auto& p : points_) {
            float depth = (p.position - cameraPos).magnitude();
            // Simple depth to color mapping (e.g., blue near, red far)
            float ratio = std::min(depth / 10.0f, 1.0f);
            p.color = Vector3(ratio, 0.5f, 1.0f - ratio);
        }
    }

    void update(float deltaTime) override {
        // Handle dynamic updates, animations, etc.
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_POINT_CLOUD_RENDERER_H
