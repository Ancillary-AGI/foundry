#include "../../include/GameEngine/systems/ScriptingSystem.h"
#include "../../include/GameEngine/core/SystemImpl.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace FoundryEngine {

// Lua Script Engine Implementation
class LuaScriptEngineImpl : public SystemImplBase<LuaScriptEngineImpl> {
private:
    // In a real implementation, this would be lua_State* L;
    std::unordered_map<std::string, std::string> loadedScripts_;
    std::unordered_map<std::string, ScriptFunction> scriptFunctions_;
    std::vector<std::string> activeScripts_;

    int scriptsExecuted_ = 0;
    float totalExecutionTime_ = 0.0f;

    friend class SystemImplBase<LuaScriptEngineImpl>;

    bool onInitialize() override {
        std::cout << "Lua Script Engine initialized" << std::endl;

        // In a real implementation, this would initialize Lua:
        // L = luaL_newstate();
        // luaL_openlibs(L);

        return true;
    }

    void onShutdown() override {
        loadedScripts_.clear();
        scriptFunctions_.clear();
        activeScripts_.clear();

        // In a real implementation: lua_close(L);
        std::cout << "Lua Script Engine shutdown" << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Execute update functions for all active scripts
        for (const auto& scriptName : activeScripts_) {
            executeScriptFunction(scriptName, "update", deltaTime);
        }
    }

    bool executeScriptFunction(const std::string& scriptName, const std::string& functionName, float deltaTime = 0.0f) {
        auto scriptIt = loadedScripts_.find(scriptName);
        if (scriptIt == loadedScripts_.end()) {
            return false;
        }

        // In a real implementation, this would:
        // 1. Get the Lua function from the registry
        // 2. Push parameters onto the stack
        // 3. Call lua_pcall
        // 4. Handle return values and errors

        scriptsExecuted_++;
        totalExecutionTime_ += 0.001f; // Mock execution time

        return true;
    }

public:
    LuaScriptEngineImpl() : SystemImplBase("LuaScriptEngine") {}

    std::string getStatistics() const override {
        char buffer[256];
        snprintf(buffer, sizeof(buffer),
                 "Script Stats - Scripts: %zu active, Executed: %d, Avg Time: %.3fms",
                 activeScripts_.size(), scriptsExecuted_,
                 scriptsExecuted_ > 0 ? (totalExecutionTime_ * 1000.0f) / scriptsExecuted_ : 0.0f);
        return std::string(buffer);
    }

    bool loadScript(const std::string& name, const std::string& scriptCode) {
        // In a real implementation, this would compile and load the Lua script
        loadedScripts_[name] = scriptCode;

        // Try to execute initialization function
        if (executeScriptFunction(name, "init")) {
            activeScripts_.push_back(name);
            return true;
        }

        return false;
    }

    bool unloadScript(const std::string& name) {
        auto it = std::find(activeScripts_.begin(), activeScripts_.end(), name);
        if (it != activeScripts_.end()) {
            activeScripts_.erase(it);
        }

        loadedScripts_.erase(name);
        scriptFunctions_.erase(name);

        return true;
    }

    bool callFunction(const std::string& scriptName, const std::string& functionName) {
        return executeScriptFunction(scriptName, functionName);
    }

    bool callFunctionWithArgs(const std::string& scriptName, const std::string& functionName,
                             const std::vector<ScriptValue>& args) {
        // In a real implementation, this would push args to Lua stack
        return executeScriptFunction(scriptName, functionName);
    }

    ScriptValue getGlobalVariable(const std::string& scriptName, const std::string& variableName) {
        // In a real implementation, this would get a Lua global variable
        return ScriptValue(); // Return nil/empty value
    }

    bool setGlobalVariable(const std::string& scriptName, const std::string& variableName, const ScriptValue& value) {
        // In a real implementation, this would set a Lua global variable
        return true;
    }

    std::vector<std::string> getLoadedScripts() const {
        std::vector<std::string> scripts;
        for (const auto& pair : loadedScripts_) {
            scripts.push_back(pair.first);
        }
        return scripts;
    }

    std::vector<std::string> getActiveScripts() const {
        return activeScripts_;
    }

    void registerFunction(const std::string& scriptName, const std::string& functionName, ScriptFunction function) {
        scriptFunctions_[scriptName + "." + functionName] = function;
    }

    void unregisterFunction(const std::string& scriptName, const std::string& functionName) {
        scriptFunctions_.erase(scriptName + "." + functionName);
    }

    bool hasScript(const std::string& name) const {
        return loadedScripts_.find(name) != loadedScripts_.end();
    }

    bool isScriptActive(const std::string& name) const {
        return std::find(activeScripts_.begin(), activeScripts_.end(), name) != activeScripts_.end();
    }
};

LuaScriptEngine::LuaScriptEngine() : impl_(std::make_unique<LuaScriptEngineImpl>()) {}
LuaScriptEngine::~LuaScriptEngine() = default;

bool LuaScriptEngine::initialize() { return impl_->initialize(); }
void LuaScriptEngine::shutdown() { impl_->shutdown(); }
void LuaScriptEngine::update(float deltaTime) { impl_->update(deltaTime); }

bool LuaScriptEngine::loadScript(const std::string& name, const std::string& scriptCode) { return impl_->loadScript(name, scriptCode); }
bool LuaScriptEngine::unloadScript(const std::string& name) { return impl_->unloadScript(name); }
bool LuaScriptEngine::callFunction(const std::string& scriptName, const std::string& functionName) { return impl_->callFunction(scriptName, functionName); }
bool LuaScriptEngine::callFunctionWithArgs(const std::string& scriptName, const std::string& functionName, const std::vector<ScriptValue>& args) { return impl_->callFunctionWithArgs(scriptName, functionName, args); }
ScriptValue LuaScriptEngine::getGlobalVariable(const std::string& scriptName, const std::string& variableName) { return impl_->getGlobalVariable(scriptName, variableName); }
bool LuaScriptEngine::setGlobalVariable(const std::string& scriptName, const std::string& variableName, const ScriptValue& value) { return impl_->setGlobalVariable(scriptName, variableName, value); }
std::vector<std::string> LuaScriptEngine::getLoadedScripts() const { return impl_->getLoadedScripts(); }
std::vector<std::string> LuaScriptEngine::getActiveScripts() const { return impl_->getActiveScripts(); }
void LuaScriptEngine::registerFunction(const std::string& scriptName, const std::string& functionName, ScriptFunction function) { impl_->registerFunction(scriptName, functionName, function); }
void LuaScriptEngine::unregisterFunction(const std::string& scriptName, const std::string& functionName) { impl_->unregisterFunction(scriptName, functionName); }
bool LuaScriptEngine::hasScript(const std::string& name) const { return impl_->hasScript(name); }
bool LuaScriptEngine::isScriptActive(const std::string& name) const { return impl_->isScriptActive(name); }

} // namespace FoundryEngine
