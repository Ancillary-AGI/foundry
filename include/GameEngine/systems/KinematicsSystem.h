#ifndef NEUTRAL_GAMEENGINE_KINEMATICS_SYSTEM_H
#define NEUTRAL_GAMEENGINE_KINEMATICS_SYSTEM_H

#include <vector>
#include <map>
#include "../../core/System.h"
#include "../../math/Vector3.h"
#include "../../math/Matrix4.h"

namespace FoundryEngine {

class KinematicsSystem : public System {
public:
    // Bone hierarchy node
    struct Bone {
        int id;
        std::string name;
        int parent = -1;
        Vector3 position; // Relative to parent
        Vector3 rotation; // Euler angles (degrees)
        Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);
        Matrix4 localTransform;
        Matrix4 worldTransform;

        Bone(int id_, const std::string& name_, int parent_ = -1)
            : id(id_), name(name_), parent(parent_) {}
    };

    struct ArticulatedFigure {
        std::vector<Bone> bones;
        std::map<std::string, int> boneIndex; // Name to index

        void addBone(int id, const std::string& name, int parent = -1) {
            bones.emplace_back(id, name, parent);
            boneIndex[name] = bones.size() - 1;
            computeLocalTransform(id);
        }

        void setBoneTransform(const std::string& boneName, const Vector3& pos, const Vector3& rot) {
            if (boneIndex.find(boneName) != boneIndex.end()) {
                int idx = boneIndex[boneName];
                bones[idx].position = pos;
                bones[idx].rotation = rot;
                computeLocalTransform(idx);
                updateWorldTransform(idx);
            }
        }
    };

    std::vector<ArticulatedFigure> figures_;

    void addFigure(int figureId) {
        if (figureId >= figures_.size()) {
            figures_.resize(figureId + 1);
        }
    }

    void forwardKinematics(int figureId, int rootBone) {
        if (figureId < figures_.size()) {
            updateWorldTransformRecursive(figureId, rootBone);
        }
    }

    // Simple CCD IK for chain
    bool inverseKinematicsCCD(int figureId, int startBone, int endBone, const Vector3& target, int maxIterations = 10) {
        if (figureId >= figures_.size()) return false;

        ArticulatedFigure& fig = figures_[figureId];
        for (int iter = 0; iter < maxIterations; ++iter) {
            // From end effector to start, adjust each joint
            int current = endBone;
            while (current != startBone && current != -1) {
                if (!adjustJoint(fig, current, startBone, endBone, target)) break;
                current = fig.bones[current].parent;
            }
        }
        return true;
    }

    Vector3 getEndEffectorPosition(int figureId, int boneId) const {
        if (figureId < figures_.size() && boneId < figures_[figureId].bones.size()) {
            return figures_[figureId].bones[boneId].worldTransform * Vector3(0, 0, 0);
        }
        return Vector3(0, 0, 0);
    }

private:
    void computeLocalTransform(int boneId, ArticulatedFigure& fig) {
        Bone& bone = fig.bones[boneId];
        // Assuming XYZ Euler rotation
        float radX = bone.rotation.x * 3.14159f / 180.0f;
        float radY = bone.rotation.y * 3.14159f / 180.0f;
        float radZ = bone.rotation.z * 3.14159f / 180.0f;

        Matrix4 rotX, rotY, rotZ;
        // Define rotation matrices (simplified, need proper implementation)
        // For brevity, assume Matrix4 has rotation methods
        bone.localTransform = Matrix4::identity();
        // Add translation, scale, rotation
        bone.localTransform.translate(bone.position);
        // bone.localTransform.rotateX(radX).rotateY(radY).rotateZ(radZ).scale(bone.scale);
    }

    void updateWorldTransformRecursive(int figureId, int boneId) {
        ArticulatedFigure& fig = figures_[figureId];
        Bone& bone = fig.bones[boneId];

        if (bone.parent == -1) {
            bone.worldTransform = bone.localTransform;
        } else {
            Bone& parent = fig.bones[bone.parent];
            bone.worldTransform = parent.worldTransform * bone.localTransform;
        }

        // Update children
        for (size_t i = 0; i < fig.bones.size(); ++i) {
            if (fig.bones[i].parent == boneId) {
                updateWorldTransformRecursive(figureId, i);
            }
        }
    }

    bool adjustJoint(ArticulatedFigure& fig, int currentBoneIdx, int startBone, int endBone, const Vector3& target) {
        Vector3 effector = getEndEffectorPosition(fig, endBone);
        Vector3 currentPos = fig.bones[currentBoneIdx].worldTransform * Vector3(0, 0, 0);
        Vector3 toEffector = effector - currentPos;
        Vector3 toTarget = target - currentPos;

        if (toEffector.magnitudeSq() < 0.001f) return false;

        // Compute angle
        float cosAngle = toEffector.dot(toTarget) / (toEffector.magnitude() * toTarget.magnitude());
        cosAngle = std::max(-1.0f, std::min(1.0f, cosAngle));
        float angle = acos(cosAngle);
        Vector3 axis = toEffector.cross(toTarget).normalized();

        // Apply rotation (simplified)
        // fig.bones[currentBoneIdx].rotation += angle * (180.0f / 3.14159f) * axis;
        // Then recompute transforms

        return true;
    }
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_KINEMATICS_SYSTEM_H
