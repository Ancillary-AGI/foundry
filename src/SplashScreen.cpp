/**
 * @file SplashScreen.cpp
 * @brief Implementation of Foundry Engine splash screen system
 */

#include "GameEngine/core/SplashScreen.h"
#include "../include/GameEngine/graphics/Renderer.h"
#include "../include/GameEngine/graphics/Texture.h"
#include "../include/GameEngine/math/Vector2.h"
#include "../include/GameEngine/math/Vector3.h"
#include "../include/GameEngine/core/Engine.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace FoundryEngine {

SplashScreen::SplashScreen()
    : startTime_(std::chrono::steady_clock::now()) {
}

SplashScreen::~SplashScreen() {
    shutdown();
}

bool SplashScreen::initialize(const SplashScreenConfig& config) {
    config_ = config;
    initialized_ = false;

    // Load logo texture first
    if (!loadLogoTexture()) {
        createDefaultBranding();
    }

    initialized_ = true;
    active_ = config.enabled;
    totalElapsedTime_ = 0.0f;
    opacity_ = 0.0f;
    loadingProgress_ = 0.0f;

    if (active_) {
        startTime_ = std::chrono::steady_clock::now();
        std::cout << "[FoundryEngine] Splash screen initialized - Branding games built with Foundry Engine" << std::endl;
    }

    return true;
}

void SplashScreen::shutdown() {
    if (logoTexture_) {
        // Texture cleanup would be handled by asset manager
        logoTexture_ = nullptr;
    }

    initialized_ = false;
    active_ = false;

    if (completionCallback_) {
        completionCallback_();
    }
}

void SplashScreen::update(float deltaTime) {
    if (!active_ || !initialized_) {
        return;
    }

    totalElapsedTime_ += deltaTime;

    // Fade in
    if (totalElapsedTime_ < config_.fadeInTime) {
        opacity_ = totalElapsedTime_ / config_.fadeInTime;
    }
    // Hold
    else if (totalElapsedTime_ < (config_.duration - config_.fadeOutTime)) {
        opacity_ = 1.0f;
    }
    // Fade out
    else if (totalElapsedTime_ < config_.duration) {
        float fadeTime = config_.duration - config_.fadeOutTime;
        opacity_ = 1.0f - ((totalElapsedTime_ - fadeTime) / config_.fadeOutTime);
        opacity_ = std::max(0.0f, opacity_);
    }
    // Complete
    else {
        active_ = false;
        if (completionCallback_) {
            completionCallback_();
        }
    }
}

void SplashScreen::render() {
    if (!active_ || !initialized_) {
        return;
    }

    // Get renderer from engine
    FoundryEngine::Engine& engine = FoundryEngine::Engine::getInstance();
    auto renderer = engine.getRenderer();
    if (!renderer) {
        // Fallback to console output for debugging
        applyFadeEffect();

        if (config_.showLogo) {
            renderLogo();
        }

        if (config_.showVersion) {
            renderVersionInfo();
        }

        if (config_.showLoadingProgress) {
            renderProgressBar();
        }

        if (!customMessage_.empty()) {
            renderCustomMessage();
        }
        return;
    }

    applyFadeEffect();

    // Set splash screen viewport (full screen)
    renderer->setViewport(0, 0, config_.screenWidth, config_.screenHeight);

    // Clear background with configured color - convert hex to RGB
    std::string bgColor = config_.backgroundColor;
    if (bgColor.length() >= 7 && bgColor[0] == '#') {
        // Parse hex color (simplified)
        float r = std::stoi(bgColor.substr(1, 2), nullptr, 16) / 255.0f;
        float g = std::stoi(bgColor.substr(3, 2), nullptr, 16) / 255.0f;
        float b = std::stoi(bgColor.substr(5, 2), nullptr, 16) / 255.0f;
        renderer->clear(r, g, b, 1.0f);
    } else {
        // Default Foundry dark background
        renderer->clear(0.11f, 0.11f, 0.11f, 1.0f);
    }

    if (config_.showLogo) {
        renderLogo();
    }

    if (config_.showVersion) {
        renderVersionInfo();
    }

    if (config_.showLoadingProgress) {
        renderProgressBar();
    }

    if (!customMessage_.empty()) {
        renderCustomMessage();
    }
}

bool SplashScreen::isActive() const {
    return active_ && initialized_;
}

float SplashScreen::getProgress() const {
    if (!initialized_ || config_.duration <= 0.0f) {
        return 1.0f;
    }
    return std::min(1.0f, totalElapsedTime_ / config_.duration);
}

void SplashScreen::setLoadingProgress(float progress) {
    loadingProgress_ = std::clamp(progress, 0.0f, 1.0f);
}

void SplashScreen::skip() {
    active_ = false;
    if (completionCallback_) {
        completionCallback_();
    }
}

void SplashScreen::setCompletionCallback(std::function<void()> callback) {
    completionCallback_ = callback;
}

void SplashScreen::setCustomMessage(const std::string& message) {
    customMessage_ = message;
}

bool SplashScreen::loadLogoTexture() {
    // Attempt to load Foundry logo texture
    // In a real implementation, this would search for:
    // - Assets/Foundry/Logo.png
    // - Assets/Textures/FoundryLogo.dds
    // - Embedded resources

    // For now, create default branding
    return false; // Force default branding for demo
}

void SplashScreen::createDefaultBranding() {
    // Create procedural Foundry logo
    // In a real engine, this would use the renderer to create a logo texture
    std::cout << "[FoundryEngine] Creating default Foundry branding" << std::endl;
}

void SplashScreen::renderLogo() {
    FoundryEngine::Engine& engine = FoundryEngine::Engine::getInstance();
    auto renderer = engine.getRenderer();

    // Scale logo appropriately based on screen size
    float scale = calculateLogoScale();

    // Center logo on screen (positioned higher for Foundry branding effect)
    Vector2 screenCenter(config_.screenWidth / 2.0f, config_.screenHeight / 2.0f - 100.0f * scale);

    if (logoTexture_ && renderer) {
        // Render actual logo texture if available
        Vector2 logoSize(400 * scale, 200 * scale);
        Vector2 logoPosition = screenCenter - logoSize * 0.5f;

        // Render texture here - would use renderer->drawTexture()
        // For now, render stylized text logo
        renderDefaultLogo(screenCenter, scale);
    } else {
        // Render stylized Foundry Engine text logo
        renderDefaultLogo(screenCenter, scale);
    }
}

void SplashScreen::renderDefaultLogo(const Vector2& position, float scale) {
    FoundryEngine::Engine& engine = FoundryEngine::Engine::getInstance();
    auto renderer = engine.getRenderer();

    if (!renderer) {
        std::cout << "[FoundryEngine] Rendering Foundry Engine Logo" << std::endl;
        return;
    }

    // Create stylized "Foundry Engine" text logo
    // This would typically use a text rendering system or pre-rendered textures

    float logoWidth = 600 * scale;
    float logoHeight = 150 * scale;

    // Position text elements
    Vector2 foundryPos(position.x - logoWidth * 0.35f, position.y);
    Vector2 enginePos(position.x + logoWidth * 0.15f, position.y);

    // Render "FOUNDRY" in bold, metallic style (would use text rendering)
    // Render "ENGINE" in sleek, futuristic style (would use text rendering)

    // For now, draw simple geometric shapes to represent the logo
    renderer->setColor(0.8f, 0.8f, 0.8f, opacity_); // Light grey with alpha

    // Draw stylized "F"
    Vector2 fStart = Vector2(foundryPos.x - 50 * scale, foundryPos.y);
    // Vertical line of F
    renderer->drawLine(fStart.x, fStart.y - 40 * scale, fStart.x, fStart.y + 40 * scale, 8 * scale);
    // Horizontal lines of F
    renderer->drawLine(fStart.x, fStart.y + 40 * scale, fStart.x + 30 * scale, fStart.y + 40 * scale, 4 * scale);
    renderer->drawLine(fStart.x, fStart.y, fStart.x + 20 * scale, fStart.y, 4 * scale);

    // Draw geometric Foundry symbol (stylized forge/spark)
    Vector2 symbolCenter(foundryPos.x - 80 * scale, foundryPos.y);
    float symbolSize = 20 * scale;

    for (int i = 0; i < 5; ++i) {
        float angle = i * (2 * 3.14159f / 5);
        float x1 = symbolCenter.x + cos(angle) * symbolSize;
        float y1 = symbolCenter.y + sin(angle) * symbolSize;
        float x2 = symbolCenter.x + cos(angle + 3.14159f) * (symbolSize * 0.6f);
        float y2 = symbolCenter.y + sin(angle + 3.14159f) * (symbolSize * 0.6f);

        renderer->setColor(1.0f, 0.8f, 0.0f, opacity_ * 0.8f); // Golden sparks
        renderer->drawLine(x1, y1, x2, y2, 2 * scale);
    }

    // Draw "ENGINE" text elements
    renderer->setColor(0.9f, 0.9f, 0.9f, opacity_); // Slightly brighter

    // Simple representation - in real implementation would use actual text rendering
    std::cout << "[FoundryEngine] Displaying Foundry Engine Logo" << std::endl;
    std::cout << "          _______  _______  _______  _______  _______           " << std::endl;
    std::cout << "         |       ||       ||       ||       ||       |          " << std::endl;
    std::cout << "         |   ____||   ____||    ___||    ___||    ___|          " << std::endl;
    std::cout << "         |  |__  ||  |__  ||   |___ |   |___ |   |___           " << std::endl;
    std::cout << "         |   __| ||   __| ||    ___||    ___||    ___|          " << std::endl;
    std::cout << "         |  |    ||  |    ||   |___ |   |___ |   |___           " << std::endl;
    std::cout << "         |__|    ||__|    ||_______||_______||_______|          " << std::endl;
    std::cout << "                                                               " << std::endl;
    std::cout << "  _______  _______  _______  _______  ______    _______  ______  " << std::endl;
    std::cout << " |       ||       ||       ||       ||    _ |  |       ||      | " << std::endl;
    std::cout << " |   ____||    ___||   ____||    ___||   | ||  |   ____||  ____| " << std::endl;
    std::cout << " |  |     |   |___ |  |__  ||   |___ |   |_||_ |  |_____|   __|__" << std::endl;
    std::cout << " |  |     |    ___||   __| ||    ___||    __  ||     __|   |____ " << std::endl;
    std::cout << " |  |____ |   |___ |  |____ |   |___ |   |  | ||   |____|       | " << std::endl;
    std::cout << " |_______||_______||_______|| _______||___|  |_||_______|_______| " << std::endl;
    std::cout << "                       |       |                                " << std::endl;
    std::cout << "                       |_______|                                " << std::endl;
}

void SplashScreen::renderVersionInfo() {
    std::string versionText = "Foundry Engine " + getEngineVersion();

    // Position version info at bottom center
    Vector2 position(config_.screenWidth / 2.0f - 100.0f, config_.screenHeight - 50.0f);

    // Renderer would handle text rendering here
    std::cout << "[FoundryEngine] " << versionText << " - Powered by Foundry Engine" << std::endl;
}

void SplashScreen::renderProgressBar() {
    // Calculate progress bar dimensions
    float barWidth = 300.0f;
    float barHeight = 4.0f;
    Vector2 barPosition(config_.screenWidth / 2.0f - barWidth / 2.0f,
                       config_.screenHeight - 25.0f);

    // Use the configured progress or animation progress
    float progress = (loadingProgress_ > 0.0f) ? loadingProgress_ : getProgress();

    // Renderer would draw the progress bar here
    std::cout << "[FoundryEngine] Loading Progress: " << std::fixed << std::setprecision(1)
              << progress * 100.0f << "%" << std::endl;
}

void SplashScreen::renderCustomMessage() {
    if (customMessage_.empty()) {
        return;
    }

    // Position custom message above progress bar
    Vector2 position(config_.screenWidth / 2.0f - 200.0f, config_.screenHeight - 80.0f);

    // Renderer would handle text rendering
    std::cout << "[FoundryEngine] " << customMessage_ << std::endl;
}

void SplashScreen::applyFadeEffect() {
    // Global opacity for the entire splash screen
    // Renderer would set global alpha here
}

float SplashScreen::calculateLogoScale() const {
    // Calculate appropriate scale based on screen resolution
    float baseWidth = 1920.0f;
    float baseHeight = 1080.0f;

    float widthScale = config_.screenWidth / baseWidth;
    float heightScale = config_.screenHeight / baseHeight;

    return std::min(widthScale, heightScale);
}

std::string SplashScreen::getEngineVersion() const {
    // Return official Foundry Engine version
    // This should be tied to build system/version info
    return "v2.0.0";
}

} // namespace FoundryEngine
