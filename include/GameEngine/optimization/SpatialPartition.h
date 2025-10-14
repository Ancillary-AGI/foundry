#ifndef FOUNDRY_GAMEENGINE_SPATIAL_PARTITION_H
#define FOUNDRY_GAMEENGINE_SPATIAL_PARTITION_H

#include <vector>
#include <map>
#include <unordered_set>
#include "../math/Vector3.h"

namespace FoundryEngine {

class SpatialPartition {
public:
    // Simple grid-based spatial hash
    struct Grid {
        float cellSize;
        std::map<std::tuple<int, int, int>, std::unordered_set<int>> cells; // cell coords -> entity IDs

        Grid(float cs = 1.0f) : cellSize(cs) {}

        void insert(int entityId, const Vector3& position) {
            auto key = getKey(position);
            cells[key].insert(entityId);
        }

        void remove(int entityId, const Vector3& position) {
            auto key = getKey(position);
            cells[key].erase(entityId);
        }

        std::vector<int> query(const Vector3& position, float radius) const {
            std::vector<int> neighbors;
            int minX = static_cast<int>((position.x - radius) / cellSize);
            int maxX = static_cast<int>((position.x + radius) / cellSize);
            int minY = static_cast<int>((position.y - radius) / cellSize);
            int maxY = static_cast<int>((position.y + radius) / cellSize);
            int minZ = static_cast<int>((position.z - radius) / cellSize);
            int maxZ = static_cast<int>((position.z + radius) / cellSize);

            for (int x = minX; x <= maxX; ++x) {
                for (int y = minY; y <= maxY; ++y) {
                    for (int z = minZ; z <= maxZ; ++z) {
                        auto key = std::make_tuple(x, y, z);
                        auto it = cells.find(key);
                        if (it != cells.end()) {
                            neighbors.insert(neighbors.end(), it->second.begin(), it->second.end());
                        }
                    }
                }
            }
            return neighbors;
        }

    private:
        std::tuple<int, int, int> getKey(const Vector3& pos) const {
            int x = static_cast<int>(pos.x / cellSize);
            int y = static_cast<int>(pos.y / cellSize);
            int z = static_cast<int>(pos.z / cellSize);
            return std::make_tuple(x, y, z);
        }
    };

    Grid grid;

    void update(int entityId, const Vector3& oldPos, const Vector3& newPos) {
        grid.remove(entityId, oldPos);
        grid.insert(entityId, newPos);
    }

    std::vector<int> getNeighbors(int entityId, const Vector3& position, float radius) {
        return grid.query(position, radius);
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_SPATIAL_PARTITION_H
