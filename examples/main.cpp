#include <iostream>
#include <memory>
#include "GameEngine/core/Engine.h"

using namespace FoundryEngine;

int main() {
    std::cout << "FoundryEngine Example Application" << std::endl;
    
    // Get engine instance
    Engine& engine = Engine::getInstance();
    
    // Initialize engine
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return -1;
    }
    
    std::cout << "Engine initialized successfully!" << std::endl;
    
    // Run engine for a few frames
    for (int i = 0; i < 10; ++i) {
        engine.update(0.016f); // 60 FPS
        engine.render();
        
        std::cout << "Frame " << i << " - Delta Time: " << engine.getDeltaTime() 
                  << ", Total Time: " << engine.getTotalTime() << std::endl;
    }
    
    // Shutdown engine
    engine.shutdown();
    
    std::cout << "Engine shutdown complete!" << std::endl;
    
    return 0;
}
