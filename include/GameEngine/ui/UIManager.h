/**
 * @file UIManager.h
 * @brief User interface management system
 */

#pragma once

#include "../core/System.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace FoundryEngine {

// Forward declarations
class UIElement;
class UICanvas;

/**
 * @class UIManager
 * @brief Manages user interface rendering and interaction
 */
class UIManager : public System {
public:
    UIManager() = default;
    virtual ~UIManager() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;

    // Canvas management
    virtual UICanvas* createCanvas(const std::string& canvasName) = 0;
    virtual bool destroyCanvas(const std::string& canvasName) = 0;
    virtual UICanvas* getCanvas(const std::string& canvasName) const = 0;

    // Element management
    virtual UIElement* createElement(const std::string& elementType,
                                   const std::string& elementName,
                                   UICanvas* canvas) = 0;
    virtual bool destroyElement(UIElement* element) = 0;

    // Input handling
    virtual bool handleInput(int x, int y, int button, bool pressed) = 0;
    virtual void handleTextInput(const std::string& text) = 0;

    // Configuration
    virtual void setResolution(int width, int height) = 0;
    virtual void setDPI(float dpi) = 0;
    virtual void setScale(float scale) = 0;

    // Themes
    virtual bool loadTheme(const std::string& themeFile) = 0;
    virtual void unloadTheme() = 0;
};

/**
 * @class DefaultUIManager
 * @brief Default UI manager implementation
 */
class DefaultUIManager : public UIManager {
public:
    bool initialize() override { return true; }
    void shutdown() override {}
    void update(float deltaTime) override {}
    void render() override {}

    UICanvas* createCanvas(const std::string& canvasName) override { return nullptr; }
    bool destroyCanvas(const std::string& canvasName) override { return false; }
    UICanvas* getCanvas(const std::string& canvasName) const override { return nullptr; }

    UIElement* createElement(const std::string& elementType,
                           const std::string& elementName,
                           UICanvas* canvas) override { return nullptr; }
    bool destroyElement(UIElement* element) override { return false; }

    bool handleInput(int x, int y, int button, bool pressed) override { return false; }
    void handleTextInput(const std::string& text) override {}

    void setResolution(int width, int height) override {}
    void setDPI(float dpi) override {}
    void setScale(float scale) override {}

    bool loadTheme(const std::string& themeFile) override { return false; }
    void unloadTheme() override {}
};

} // namespace FoundryEngine
