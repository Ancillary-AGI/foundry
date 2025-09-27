#ifndef NEUTRAL_GAMEENGINE_SPRITE_RENDERER_H
#define NEUTRAL_GAMEENGINE_SPRITE_RENDERER_H

#include <vector>
#include "../../core/System.h"
#include "../../math/Vector2.h"
#include "../../math/Vector3.h"

namespace FoundryEngine {

struct Sprite {
    Vector2 position;
    Vector2 size;
    Vector3 color = Vector3(1,1,1);
    float rotation = 0.0f; // degrees
    bool visible = true;
};

class SpriteRenderer : public System {
public:
    std::vector<Sprite> sprites_;

    void addSprite(const Vector2& pos, const Vector2& size, const Vector3& color = Vector3(1,1,1), float rot = 0.0f) {
        sprites_.push_back({pos, size, color, rot});
    }

    // Render to buffer (simple 2D orthographic projection)
    void renderToBuffer(std::vector<unsigned char>& buffer, int width, int height) {
        // Clear buffer to black
        buffer.assign(width * height * 3, 0);

        for (const auto& sprite : sprites_) {
            if (!sprite.visible) continue;

            // Simple AABB rendering (rectangular sprites)
            int left = static_cast<int>(sprite.position.x - sprite.size.x / 2);
            int right = static_cast<int>(sprite.position.x + sprite.size.x / 2);
            int top = static_cast<int>(sprite.position.y - sprite.size.y / 2);
            int bottom = static_cast<int>(sprite.position.y + sprite.size.y / 2);

            for (int y = std::max(0, top); y < std::min(height, bottom); ++y) {
                for (int x = std::max(0, left); x < std::min(width, right); ++x) {
                    int idx = (y * width + x) * 3;
                    buffer[idx] = static_cast<unsigned char>(sprite.color.x * 255);
                    buffer[idx + 1] = static_cast<unsigned char>(sprite.color.y * 255);
                    buffer[idx + 2] = static_cast<unsigned char>(sprite.color.z * 255);
                }
            }
        }
    }

    // Collision detection (2D AABB)
    bool checkCollision(const Sprite& a, const Sprite& b) {
        Vector2 amin = a.position - a.size / 2;
        Vector2 amax = a.position + a.size / 2;
        Vector2 bmin = b.position - b.size / 2;
        Vector2 bmax = b.position + b.size / 2;

        return (amin.x < bmax.x && amax.x > bmin.x) &&
               (amin.y < bmax.y && amax.y > bmin.y);
    }

    void update(float deltaTime) override {
        // Handle sprite animations, updates
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_SPRITE_RENDERER_H
