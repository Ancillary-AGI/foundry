#include "../include/GameEngine/core/Engine.h"
#include "../include/GameEngine/core/World.h"
#include "../include/GameEngine/core/Scene.h"
#include "../include/GameEngine/core/SplashScreen.h"
#include "../include/GameEngine/graphics/Renderer.h"
#include "../include/GameEngine/systems/AudioSystem.h"
#include "../include/GameEngine/systems/InputSystem.h"
#include "../include/GameEngine/systems/PhysicsSystem.h"
#include "../include/GameEngine/systems/ScriptingSystem.h"
#include "../include/GameEngine/systems/AssetSystem.h"
#include "../include/GameEngine/systems/UISystem.h"
#include "../include/GameEngine/systems/NetworkSystem.h"
#include "../include/GameEngine/systems/ProfilerSystem.h"
#include <thread>
#include <iostream>

namespace FoundryEngine {

Engine& Engine::getInstance() {
    static Engine instance;
    return instance;
}

bool Engine::initialize() {
    // Initialize splash screen first for branding
    splashScreen_ = std::make_unique<SplashScreen>();

    world_ = std::make_unique<World>();
    scenes_ = std::make_unique<SceneManager>();
    assets_ = std::make_unique<DefaultAssetManager>();
    profiler_ = std::make_unique<DefaultProfileManager>();
    
#ifdef _WIN32
    renderer_ = std::make_unique<D3D11Renderer>();
    audio_ = std::make_unique<XAudio2Manager>();
#elif defined(__linux__)
    renderer_ = std::make_unique<OpenGLRenderer>();
    audio_ = std::make_unique<OpenALAudioManager>();
#elif defined(__APPLE__)
    renderer_ = std::make_unique<OpenGLRenderer>();
    audio_ = std::make_unique<OpenALAudioManager>();
#else
    renderer_ = std::make_unique<OpenGLRenderer>();
    audio_ = std::make_unique<OpenALAudioManager>();
#endif

    input_ = std::make_unique<DefaultInputManager>();
    physics_ = std::make_unique<BulletPhysicsWorld>();
    scripting_ = std::make_unique<LuaScriptEngine>();
    ui_ = std::make_unique<UIManager>();
    network_ = std::make_unique<UDPNetworkManager>();
    
    if (!profiler_->initialize()) return false;
    if (!assets_->initialize()) return false;
    if (!renderer_->initialize()) return false;
    if (!audio_->initialize()) return false;
    if (!input_->initialize()) return false;
    if (!physics_->initialize()) return false;
    if (!scripting_->initialize()) return false;
    if (!ui_->initialize()) return false;
    if (!network_->initialize()) return false;
    
    Scene* defaultScene = scenes_->createScene("DefaultScene");
    scenes_->setActiveScene(defaultScene);
    
    lastFrameTime_ = std::chrono::high_resolution_clock::now();
    running_ = true;
    
    return true;
}

void Engine::run() {
    while (running_) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastFrameTime_);
        deltaTime_ = duration.count() / 1000000.0f;
        totalTime_ += deltaTime_;
        frameCount_++;
        lastFrameTime_ = currentTime;
        
        if (targetFPS_ > 0) {
            float targetFrameTime = 1.0f / targetFPS_;
            if (deltaTime_ < targetFrameTime) {
                auto sleepTime = std::chrono::microseconds(static_cast<long long>((targetFrameTime - deltaTime_) * 1000000));
                std::this_thread::sleep_for(sleepTime);
                deltaTime_ = targetFrameTime;
            }
        }
        
        if (!paused_) {
            profiler_->beginFrame();
            update(deltaTime_);
            render();
            profiler_->endFrame();
        }
        
        profiler_->update();
    }
}

void Engine::update(float deltaTime) {
    // Update splash screen first if active (shows progress during loading)
    if (splashScreen_ && splashScreen_->isActive()) {
        splashScreen_->update(deltaTime);
        splashScreen_->setLoadingProgress(0.5f); // Example progress (would be based on actual loading)
    }

    input_->update();
    network_->update();
    scripting_->update(deltaTime);
    physics_->step(deltaTime);
    audio_->update();

    if (scenes_->getActiveScene()) {
        scenes_->getActiveScene()->update(deltaTime);
    }
    scenes_->update(deltaTime);

    world_->update(deltaTime);
    ui_->update(deltaTime);
    assets_->update();
}

void Engine::render() {
    renderer_->beginFrame();

    // Render splash screen first if active (takes priority over regular UI)
    if (splashScreen_ && splashScreen_->isActive()) {
        splashScreen_->render();
    } else {
        // Render regular UI and game content
        ui_->render();
    }

    renderer_->endFrame();
    renderer_->present();
}

void Engine::shutdown() {
    running_ = false;
    
    if (network_) network_->shutdown();
    if (ui_) ui_->shutdown();
    if (scripting_) scripting_->shutdown();
    if (physics_) physics_->shutdown();
    if (input_) input_->shutdown();
    if (audio_) audio_->shutdown();
    if (renderer_) renderer_->shutdown();
    if (assets_) assets_->shutdown();
    if (profiler_) profiler_->shutdown();
    
    network_.reset();
    ui_.reset();
    scripting_.reset();
    physics_.reset();
    input_.reset();
    audio_.reset();
    renderer_.reset();
    scenes_.reset();
    assets_.reset();
    world_.reset();
    profiler_.reset();
    splashScreen_.reset();
}

} // namespace FoundryEngine
