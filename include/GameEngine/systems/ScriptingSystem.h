#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace FoundryEngine {

class Entity;
class Component;

enum class ScriptLanguage {
    LUA,
    PYTHON,
    CSHARP,
    JAVASCRIPT,
    WASM
};

class ScriptContext {
public:
    virtual ~ScriptContext() = default;
    virtual bool execute(const std::string& code) = 0;
    virtual bool executeFile(const std::string& path) = 0;
    virtual void setGlobal(const std::string& name, void* value) = 0;
    virtual void* getGlobal(const std::string& name) = 0;
    virtual bool callFunction(const std::string& name, const std::vector<void*>& args = {}) = 0;
    virtual void registerFunction(const std::string& name, std::function<void()> func) = 0;
    virtual void registerClass(const std::string& name, void* classPtr) = 0;
};

class Script {
public:
    virtual ~Script() = default;
    virtual bool load(const std::string& path) = 0;
    virtual bool compile() = 0;
    virtual bool isCompiled() const = 0;
    virtual ScriptLanguage getLanguage() const = 0;
    virtual const std::string& getSource() const = 0;
    virtual void setSource(const std::string& source) = 0;
    virtual ScriptContext* getContext() = 0;
    
    virtual void onStart() {}
    virtual void onUpdate(float deltaTime) {}
    virtual void onFixedUpdate(float fixedDeltaTime) {}
    virtual void onLateUpdate(float deltaTime) {}
    virtual void onDestroy() {}
    virtual void onCollisionEnter(Entity* other) {}
    virtual void onCollisionExit(Entity* other) {}
    virtual void onTriggerEnter(Entity* other) {}
    virtual void onTriggerExit(Entity* other) {}
};

class ScriptEngine {
public:
    virtual ~ScriptEngine() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;
    
    virtual ScriptContext* createContext(ScriptLanguage language) = 0;
    virtual void destroyContext(ScriptContext* context) = 0;
    
    virtual Script* loadScript(const std::string& path, ScriptLanguage language) = 0;
    virtual Script* createScript(const std::string& source, ScriptLanguage language) = 0;
    virtual void destroyScript(Script* script) = 0;
    
    virtual bool compileScript(Script* script) = 0;
    virtual bool executeScript(Script* script) = 0;
    
    virtual void registerEngineAPI() = 0;
    virtual void registerFunction(const std::string& name, std::function<void()> func) = 0;
    virtual void registerClass(const std::string& name, void* classPtr) = 0;
    
    virtual void setScriptDirectory(const std::string& directory) = 0;
    virtual std::string getScriptDirectory() const = 0;
    
    virtual void enableHotReload(bool enable) = 0;
    virtual bool isHotReloadEnabled() const = 0;
    
    virtual void reloadAllScripts() = 0;
    virtual void reloadScript(Script* script) = 0;
    
    virtual std::vector<std::string> getAvailableScripts() const = 0;
    virtual std::vector<ScriptLanguage> getSupportedLanguages() const = 0;
};

class LuaScriptEngine : public ScriptEngine {
public:
    bool initialize() override;
    void shutdown() override;
    void update(float deltaTime) override;
    ScriptContext* createContext(ScriptLanguage language) override;
    void destroyContext(ScriptContext* context) override;
    Script* loadScript(const std::string& path, ScriptLanguage language) override;
    Script* createScript(const std::string& source, ScriptLanguage language) override;
    void destroyScript(Script* script) override;
    bool compileScript(Script* script) override;
    bool executeScript(Script* script) override;
    void registerEngineAPI() override;
    void registerFunction(const std::string& name, std::function<void()> func) override;
    void registerClass(const std::string& name, void* classPtr) override;
    void setScriptDirectory(const std::string& directory) override;
    std::string getScriptDirectory() const override;
    void enableHotReload(bool enable) override;
    bool isHotReloadEnabled() const override;
    void reloadAllScripts() override;
    void reloadScript(Script* script) override;
    std::vector<std::string> getAvailableScripts() const override;
    std::vector<ScriptLanguage> getSupportedLanguages() const override;
};

class PythonScriptEngine : public ScriptEngine {
public:
    bool initialize() override;
    void shutdown() override;
    void update(float deltaTime) override;
    ScriptContext* createContext(ScriptLanguage language) override;
    void destroyContext(ScriptContext* context) override;
    Script* loadScript(const std::string& path, ScriptLanguage language) override;
    Script* createScript(const std::string& source, ScriptLanguage language) override;
    void destroyScript(Script* script) override;
    bool compileScript(Script* script) override;
    bool executeScript(Script* script) override;
    void registerEngineAPI() override;
    void registerFunction(const std::string& name, std::function<void()> func) override;
    void registerClass(const std::string& name, void* classPtr) override;
    void setScriptDirectory(const std::string& directory) override;
    std::string getScriptDirectory() const override;
    void enableHotReload(bool enable) override;
    bool isHotReloadEnabled() const override;
    void reloadAllScripts() override;
    void reloadScript(Script* script) override;
    std::vector<std::string> getAvailableScripts() const override;
    std::vector<ScriptLanguage> getSupportedLanguages() const override;
};

class MonoScriptEngine : public ScriptEngine {
public:
    bool initialize() override;
    void shutdown() override;
    void update(float deltaTime) override;
    ScriptContext* createContext(ScriptLanguage language) override;
    void destroyContext(ScriptContext* context) override;
    Script* loadScript(const std::string& path, ScriptLanguage language) override;
    Script* createScript(const std::string& source, ScriptLanguage language) override;
    void destroyScript(Script* script) override;
    bool compileScript(Script* script) override;
    bool executeScript(Script* script) override;
    void registerEngineAPI() override;
    void registerFunction(const std::string& name, std::function<void()> func) override;
    void registerClass(const std::string& name, void* classPtr) override;
    void setScriptDirectory(const std::string& directory) override;
    std::string getScriptDirectory() const override;
    void enableHotReload(bool enable) override;
    bool isHotReloadEnabled() const override;
    void reloadAllScripts() override;
    void reloadScript(Script* script) override;
    std::vector<std::string> getAvailableScripts() const override;
    std::vector<ScriptLanguage> getSupportedLanguages() const override;
};

} // namespace FoundryEngine
