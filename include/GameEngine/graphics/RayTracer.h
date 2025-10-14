#ifndef FOUNDRY_GAMEENGINE_RAY_TRACER_H
#define FOUNDRY_GAMEENGINE_RAY_TRACER_H

#include <vector>
#include <memory>
#include "../../core/System.h"
#include "../../math/Vector3.h"
#include "../../math/Matrix4.h"

namespace FoundryEngine {

// BVH Node for acceleration structure
struct BVHNode {
    Vector3 minBounds;
    Vector3 maxBounds;
    BVHNode* left = nullptr;
    BVHNode* right = nullptr;
    int objectIndex = -1;
    bool isLeaf() const { return left == nullptr && right == nullptr; }
};

// Surface Area Heuristic (SAH) based BVH
class BVH {
public:
    BVH(std::vector<Sphere>& spheres, std::vector<Plane>& planes);
    ~BVH();

    bool intersect(const Ray& ray, HitRecord& hit) const;

private:
    BVHNode* root = nullptr;
    std::vector<Sphere>& spheres_;
    std::vector<Plane>& planes_;

    BVHNode* build(std::vector<std::pair<int, float>>& objects, int axis);
    bool traverse(const BVHNode* node, const Ray& ray, HitRecord& hit, float tMin, float tMax) const;
    bool aabbIntersect(const Vector3& minB, const Vector3& maxB, const Ray& ray) const;
    float sahCost(const std::vector<std::pair<int, float>>& objects, int split, int axis) const;
};

class RayTracer : public System {
public:
    struct Ray {
        Vector3 origin;
        Vector3 direction;

        Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d.normalized()) {}
    };

    struct HitRecord {
        float t;
        Vector3 point;
        Vector3 normal;
        bool hit = false;
    };

    struct Material {
        Vector3 color;
        float reflectivity = 0.0f;
        float transparency = 0.0f;
        float refractiveIndex = 1.5f;

        Material(const Vector3& c = Vector3(1,1,1)) : color(c) {}
    };

    struct Sphere {
        Vector3 center;
        float radius;
        Material material;

        Sphere(const Vector3& c, float r, const Material& mat = Material()) : center(c), radius(r), material(mat) {}
    };

    struct Plane {
        Vector3 point; // A point on the plane
        Vector3 normal; // Normal to the plane
        Material material;

        Plane(const Vector3& p, const Vector3& n, const Material& mat = Material()) : point(p), normal(n.normalized()), material(mat) {}
    };

    std::vector<Sphere> spheres_;
    std::vector<Plane> planes_;
    Vector3 backgroundColor_ = Vector3(0.2f, 0.3f, 0.8f);
    int maxBounces_ = 5;

    // Add objects
    void addSphere(const Sphere& sphere) { spheres_.push_back(sphere); }
    void addPlane(const Plane& plane) { planes_.push_back(plane); }

    // Ray intersection with scene
    HitRecord intersect(const Ray& ray) {
        HitRecord closestHit;
        closestHit.t = INFINITY;
        closestHit.hit = false;

        // Test spheres
        for (const auto& sphere : spheres_) {
            HitRecord hit = intersectSphere(ray, sphere);
            if (hit.hit && hit.t < closestHit.t) {
                closestHit = hit;
                closestHit.normal = (hit.point - sphere.center).normalized();
            }
        }

        // Test planes
        for (const auto& plane : planes_) {
            HitRecord hit = intersectPlane(ray, plane);
            if (hit.hit && hit.t < closestHit.t) {
                closestHit = hit;
                closestHit.normal = plane.normal;
            }
        }

        return closestHit;
    }

    // Trace ray with recursive reflections/refractions
    Vector3 trace(const Ray& ray, int depth = 0) {
        if (depth > maxBounces_) return backgroundColor_;

        HitRecord hit = intersect(ray);
        if (!hit.hit) return backgroundColor_;

        Vector3 localColor = shade(hit);
        Vector3 reflectionColor = Vector3(0,0,0);
        Vector3 refractionColor = Vector3(0,0,0);

        // Reflection
        if (hit.material.reflectivity > 0) {
            Vector3 reflectDir = reflect(ray.direction, hit.normal);
            Ray reflectRay(hit.point + hit.normal * 0.001f, reflectDir);
            reflectionColor = trace(reflectRay, depth + 1) * hit.material.reflectivity;
        }

        // Refraction (basic)
        if (hit.material.transparency > 0) {
            Vector3 refractDir = refract(ray.direction, hit.normal, 1.0f, hit.material.refractiveIndex);
            if (refractDir.magnitudeSq() > 0) {
                Ray refractRay(hit.point - hit.normal * 0.001f, refractDir);
                refractionColor = trace(refractRay, depth + 1) * hit.material.transparency;
            } else {
                refractionColor = reflectionColor;
            }
        }

        return localColor * (1 - hit.material.reflectivity - hit.material.transparency) +
               reflectionColor + refractionColor;
    }

    void update(float deltaTime) override {
        // Rendering would be called separately, e.g., for each pixel
    }

private:
    HitRecord intersectSphere(const Ray& ray, const Sphere& sphere) {
        Vector3 oc = ray.origin - sphere.center;
        float a = ray.direction.dot(ray.direction);
        float b = 2.0f * oc.dot(ray.direction);
        float c = oc.dot(oc) - sphere.radius * sphere.radius;
        float discriminant = b*b - 4*a*c;

        if (discriminant < 0) return {};

        float t1 = (-b - sqrt(discriminant)) / (2*a);
        float t2 = (-b + sqrt(discriminant)) / (2*a);

        HitRecord hit;
        hit.hit = true;
        hit.t = (t1 > 0 && t2 > 0) ? std::min(t1, t2) : std::max(t1, t2);
        if (hit.t > 0) {
            hit.point = ray.origin + ray.direction * hit.t;
            return hit;
        }
        return {};
    }

    HitRecord intersectPlane(const Ray& ray, const Plane& plane) {
        float denom = plane.normal.dot(ray.direction);
        if (std::abs(denom) < 1e-6) return {}; // Parallel

        float t = (plane.point - ray.origin).dot(plane.normal) / denom;
        if (t > 0) {
            HitRecord hit;
            hit.hit = true;
            hit.t = t;
            hit.point = ray.origin + ray.direction * t;
            return hit;
        }
        return {};
    }

    Vector3 shade(const HitRecord& hit) {
        // Simple directional lighting
        Vector3 lightDir = Vector3(1,1,1).normalized();
        float diff = std::max(0.0f, hit.normal.dot(lightDir));
        return hit.material.color * diff * 0.8f + hit.material.color * 0.2f; // Ambient + diffuse
    }

    Vector3 reflect(const Vector3& v, const Vector3& n) {
        return v - 2 * v.dot(n) * n;
    }

    Vector3 refract(const Vector3& v, const Vector3& n, float n1, float n2) {
        float ratio = n1 / n2;
        float cosI = -n.dot(v);
        float sinT2 = ratio * ratio * (1 - cosI * cosI);
        if (sinT2 > 1.0) return Vector3(0,0,0); // Total internal reflection
        float cosT = sqrt(1.0 - sinT2);
        return ratio * v + (ratio * cosI - cosT) * n;
    }
};

} // namespace FoundryEngine

#endif // FOUNDRY_GAMEENGINE_RAY_TRACER_H
