/**
 * @file ScriptingSystem.h
 * @brief Cross-platform scripting system
 */

#pragma once

#include "../core/System.h"
#include <memory>
#include <string>
#include <functional>

namespace FoundryEngine {

/**
 * @class ScriptEngine
 * @brief Base scripting engine interface
 */
class ScriptEngine : public System {
public:
    ScriptEngine() = default;
    virtual ~ScriptEngine() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;

    // Script execution
    virtual bool loadScript(const std::string& filename) = 0;
    virtual bool executeScript(const std::string& script) = 0;
    virtual bool callFunction(const std::string& functionName) = 0;

    // Variable binding
    virtual void setGlobalVariable(const std::string& name, int value) = 0;
    virtual void setGlobalVariable(const std::string& name, float value) = 0;
    virtual void setGlobalVariable(const std::string& name, const std::string& value) = 0;

    virtual int getGlobalInt(const std::string& name) const = 0;
    virtual float getGlobalFloat(const std::string& name) const = 0;
    virtual std::string getGlobalString(const std::string& name) const = 0;

    // Function binding
    virtual void registerFunction(const std::string& name,
                                std::function<void()> func) = 0;
    virtual void registerFunction(const std::string& name,
                                std::function<int()> func) = 0;
    virtual void registerFunction(const std::string& name,
                                std::function<float()> func) = 0;
};

/**
 * @class LuaScriptEngine
 * @brief Lua scripting engine implementation
 */
class LuaScriptEngine : public ScriptEngine {
public:
    bool initialize() override { return true; }
    void shutdown() override {}
    void update(float deltaTime) override {}

    bool loadScript(const std::string& filename) override { return false; }
    bool executeScript(const std::string& script) override { return false; }
    bool callFunction(const std::string& functionName) override { return false; }

    void setGlobalVariable(const std::string& name, int value) override {}
    void setGlobalVariable(const std::string& name, float value) override {}
    void setGlobalVariable(const std::string& name, const std::string& value) override {}

    int getGlobalInt(const std::string& name) const override { return 0; }
    float getGlobalFloat(const std::string& name) const override { return 0.0f; }
    std::string getGlobalString(const std::string& name) const override { return ""; }

    void registerFunction(const std::string& name, std::function<void()> func) override {}
    void registerFunction(const std::string& name, std::function<int()> func) override {}
    void registerFunction(const std::string& name, std::function<float()> func) override {}
};

} // namespace FoundryEngine
