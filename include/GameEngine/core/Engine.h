#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>

namespace FoundryEngine {

class World;
class Renderer;
class AudioManager;
class InputManager;
class PhysicsWorld;
class ScriptEngine;
class AssetManager;
class SceneManager;
class UIManager;
class NetworkManager;
class ProfileManager;

class Engine {
public:
    static Engine& getInstance();
    
    bool initialize();
    void run();
    void shutdown();
    
    // Core systems
    World* getWorld() const { return world_.get(); }
    Renderer* getRenderer() const { return renderer_.get(); }
    AudioManager* getAudio() const { return audio_.get(); }
    InputManager* getInput() const { return input_.get(); }
    PhysicsWorld* getPhysics() const { return physics_.get(); }
    ScriptEngine* getScripting() const { return scripting_.get(); }
    AssetManager* getAssets() const { return assets_.get(); }
    SceneManager* getScenes() const { return scenes_.get(); }
    UIManager* getUI() const { return ui_.get(); }
    NetworkManager* getNetwork() const { return network_.get(); }
    ProfileManager* getProfiler() const { return profiler_.get(); }
    
    // Time management
    float getDeltaTime() const { return deltaTime_; }
    float getTotalTime() const { return totalTime_; }
    uint64_t getFrameCount() const { return frameCount_; }
    
    // Engine control
    void setTargetFPS(int fps) { targetFPS_ = fps; }
    void pause() { paused_ = true; }
    void resume() { paused_ = false; }
    void quit() { running_ = false; }
    
private:
    Engine() = default;
    ~Engine() = default;
    
    void update(float deltaTime);
    void render();
    
    std::unique_ptr<World> world_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<AudioManager> audio_;
    std::unique_ptr<InputManager> input_;
    std::unique_ptr<PhysicsWorld> physics_;
    std::unique_ptr<ScriptEngine> scripting_;
    std::unique_ptr<AssetManager> assets_;
    std::unique_ptr<SceneManager> scenes_;
    std::unique_ptr<UIManager> ui_;
    std::unique_ptr<NetworkManager> network_;
    std::unique_ptr<ProfileManager> profiler_;
    
    bool running_ = false;
    bool paused_ = false;
    int targetFPS_ = 60;
    float deltaTime_ = 0.0f;
    float totalTime_ = 0.0f;
    uint64_t frameCount_ = 0;
    
    std::chrono::high_resolution_clock::time_point lastFrameTime_;
};

} // namespace FoundryEngine
