#ifndef NEUTRAL_GAMEENGINE_TRANSFORM_COMPONENT_H
#define NEUTRAL_GAMEENGINE_TRANSFORM_COMPONENT_H

#include "Component.h"
#include "../math/Vector3.h"
#include "../math/Matrix4.h"

namespace FoundryEngine {

class TransformComponent : public Component {
public:
    Vector3 position;
    Vector3 rotation; // Euler angles
    Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);

    TransformComponent(const Vector3& pos = Vector3(), const Vector3& rot = Vector3(), const Vector3& scl = Vector3(1,1,1))
        : position(pos), rotation(rot), scale(scl) {}

    Matrix4 getTransformMatrix() const {
        Matrix4 mat = Matrix4::identity();
        mat.translate(position);
        // Add rotation if implemented
        mat.scale(scale);
        return mat;
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_TRANSFORM_COMPONENT_H
