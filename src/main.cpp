#include <iostream>
#include "../include/GameEngine/systems/NBodySystem.h"
#include "../include/GameEngine/systems/FluidSystem.h"
#include "../include/GameEngine/graphics/RayTracer.h"
#include "../include/GameEngine/math/Vector3.h"
#include "../include/GameEngine/systems/AISystem.h"
#include "../include/GameEngine/systems/ClothSystem.h"
#include "../include/GameEngine/systems/FluidSystem.h"
#include "../include/GameEngine/systems/SpringSystem.h"

using namespace FoundryEngine;

int main() {
    std::cout << "Next-Generation Game Engine Advanced Systems Demo" << std::endl;

    // Demonstrate N-Body Physics
    std::cout << "\n=== N-Body Physics Demo ===" << std::endl;
    NBodySystem nbody;
    nbody.addBody(Vector3(0, 0, 0), Vector3(0, 0, 0), 1e10); // Central mass
    nbody.addBody(Vector3(10, 0, 0), Vector3(0, 5, 0), 1e6); // Orbiting body
    nbody.update(0.01f); // Simulate one step
    std::cout << "Body 1 position after simulation: ("
              << nbody.bodies_[0].position.x << ", "
              << nbody.bodies_[0].position.y << ", "
              << nbody.bodies_[0].position.z << ")" << std::endl;

    // Demonstrate Fluid Simulation
    std::cout << "\n=== Fluid Simulation Demo ===" << std::endl;
    FluidSystem fluid;
    fluid.addParticle(Vector3(0, 0, 0));
    fluid.addParticle(Vector3(0.1f, 0, 0));
    fluid.update(0.005f);
    std::cout << "Particle 0 position: ("
              << fluid.particles_[0].position.x << ", "
              << fluid.particles_[0].position.y << ", "
              << fluid.particles_[0].position.z << ")" << std::endl;

    // Demonstrate Ray Tracing
    std::cout << "\n=== Ray Tracing Demo ===" << std::endl;
    RayTracer rayTracer;
    rayTracer.addSphere(RayTracer::Sphere(Vector3(0, 0, -3), 1.0f, RayTracer::Material(Vector3(1, 0, 0))));
    rayTracer.addPlane(RayTracer::Plane(Vector3(0, -1, 0), Vector3(0, 1, 0)));
    Vector3 color = rayTracer.trace(RayTracer::Ray(Vector3(0, 0, 0), Vector3(0, 0, -1)));
    std::cout << "Pixel color through ray tracing: ("
              << color.x << ", " << color.y << ", " << color.z << ")" << std::endl;

    // Demonstrate AI Systems
    std::cout << "\n=== AI Systems Demo ===" << std::endl;
    AISystem aiSystem;

    // Neural Network demo
    std::vector<float> inputs = {0.5f, 0.8f};
    auto output = aiSystem.neuralNetwork.feedforward(inputs);
    std::cout << "Neural Network output: " << output[0] << std::endl;

    // Train on XOR
    aiSystem.neuralNetwork.train({0,0}, {0});
    aiSystem.neuralNetwork.train({0,1}, {1});
    aiSystem.neuralNetwork.train({1,0}, {1});
    aiSystem.neuralNetwork.train({1,1}, {0});

    output = aiSystem.neuralNetwork.feedforward({0, 0});
    std::cout << "XOR(0,0): " << output[0] << std::endl;
    output = aiSystem.neuralNetwork.feedforward({1, 1});
    std::cout << "XOR(1,1): " << output[0] << std::endl;

    // Flocking demo
    std::cout << "\nFlocking simulation:" << std::endl;
    aiSystem.addBoid(Vector3(0, 0, 0), Vector3(1, 0, 0));
    aiSystem.addBoid(Vector3(10, 0, 10), Vector3(0, 0, 1));
    aiSystem.addBoid(Vector3(-5, 0, 5), Vector3(-1, 0, -1));
    aiSystem.updateFlocking(0.1f);
    std::cout << "Boid 0 position after flocking: (" << aiSystem.boids[0].position.x << ", " << aiSystem.boids[0].position.y << ", " << aiSystem.boids[0].position.z << ")" << std::endl;

    // Pathfinding demo
    aiSystem.createGrid(10, 10, 1.0f);
    auto path = aiSystem.findPath(Vector3(0, 0, 0), Vector3(9, 0, 9));
    std::cout << "\nA* Pathfinding: Path length = " << path.size() << std::endl;

    // Genetic Algorithm demo
    aiSystem.initializePopulation(10, 5);
    aiSystem.evaluateFitness([](const std::vector<float>& genes) {
        float sum = 0;
        for (float g : genes) sum += g * g; // Minimize sum of squares
        return 1.0f / (1.0f + sum); // Fitness
    });
    std::cout << "Best GA fitness: " << aiSystem.population[0].fitness << std::endl;

    std::cout << "\nAI functionalities implemented and demonstrated!" << std::endl;

    std::cout << "\nAdvanced systems initialized successfully!" << std::endl;
    std::cout << "Engine ready for complex simulations." << std::endl;

    return 0;
}
