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
        // Create procedural Foundry logo using renderer primitives
        // In a production engine, this would load a proper logo texture
        std::cout << "[FoundryEngine] Creating Foundry Engine procedural logo" << std::endl;
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

    // Create sophisticated "Foundry Engine" logo using debug drawing primitives
    // This creates a professional-looking logo similar to Unity's splash screen
    float logoWidth = 600 * scale;
    float logoHeight = 150 * scale;

    // Position text elements
    Vector2 logoCenter(position.x, position.y);

    // Draw main "FOUNDRY" text using connected lines and blocks
    // F
    Vector3 lineColor(0.9f, 0.9f, 0.9f); // Bright white
    float textScale = 1.5f;
    float letterSpacing = 60 * scale * textScale;
    float baseY = logoCenter.y;

    // F - Vertical stroke
    renderer->drawDebugLine(
        Vector3(logoCenter.x - 200 * scale, baseY - 30 * scale, 0),
        Vector3(logoCenter.x - 200 * scale, baseY + 30 * scale, 0),
        lineColor
    );
    // F - Top horizontal
    renderer->drawDebugLine(
        Vector3(logoCenter.x - 200 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x - 170 * scale, baseY + 30 * scale, 0),
        lineColor
    );
    // F - Middle horizontal
    renderer->drawDebugLine(
        Vector3(logoCenter.x - 200 * scale, baseY + 5 * scale, 0),
        Vector3(logoCenter.x - 175 * scale, baseY + 5 * scale, 0),
        lineColor
    );

    // O - Draw as outline
    Vector3 oCenter(logoCenter.x - 120 * scale, baseY, 0);
    float oRadius = 18 * scale;
    // Draw circular O using debug lines
    for (int i = 0; i < 32; ++i) {
        float angle1 = (i / 32.0f) * 2 * 3.14159f;
        float angle2 = ((i + 1) / 32.0f) * 2 * 3.14159f;
        renderer->drawDebugLine(
            Vector3(oCenter.x + cos(angle1) * oRadius, oCenter.y + sin(angle1) * oRadius, 0),
            Vector3(oCenter.x + cos(angle2) * oRadius, oCenter.y + sin(angle2) * oRadius, 0),
            lineColor
        );
    }

    // U - Two verticals connected by bottom
    renderer->drawDebugLine(
        Vector3(logoCenter.x - 90 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x - 90 * scale, baseY - 30 * scale, 0),
        lineColor
    );
    renderer->drawDebugLine(
        Vector3(logoCenter.x - 60 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x - 60 * scale, baseY - 30 * scale, 0),
        lineColor
    );
    renderer->drawDebugLine(
        Vector3(logoCenter.x - 90 * scale, baseY - 30 * scale, 0),
        Vector3(logoCenter.x - 60 * scale, baseY - 30 * scale, 0),
        lineColor
    );

    // N - Diagonal
    renderer->drawDebugLine(
        Vector3(logoCenter.x - 30 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x - 30 * scale, baseY - 30 * scale, 0),
        lineColor
    );
    renderer->drawDebugLine(
        Vector3(logoCenter.x - 30 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x + 5 * scale, baseY - 30 * scale, 0),
        lineColor
    );
    renderer->drawDebugLine(
        Vector3(logoCenter.x + 5 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x + 5 * scale, baseY - 30 * scale, 0),
        lineColor
    );

    // D - Vertical with curved right side
    renderer->drawDebugLine(
        Vector3(logoCenter.x + 35 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x + 35 * scale, baseY - 30 * scale, 0),
        lineColor
    );
    // D curve (simplified)
    Vector3 dCenter(logoCenter.x + 65 * scale, baseY, 0);
    float dRadius = 35 * scale;
    for (int i = 0; i < 16; ++i) {
        float angle1 = (i / 16.0f) * 3.14159f - 3.14159f/2;
        float angle2 = ((i + 1) / 16.0f) * 3.14159f - 3.14159f/2;
        renderer->drawDebugLine(
            Vector3(dCenter.x + cos(angle1) * dRadius, dCenter.y + sin(angle1) * dRadius, 0),
            Vector3(dCenter.x + cos(angle2) * dRadius, dCenter.y + sin(angle2) * dRadius, 0),
            lineColor
        );
    }

    // R - Vertical with diagonal
    renderer->drawDebugLine(
        Vector3(logoCenter.x + 95 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x + 95 * scale, baseY - 30 * scale, 0),
        lineColor
    );
    renderer->drawDebugLine(
        Vector3(logoCenter.x + 95 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x + 125 * scale, baseY + 30 * scale, 0),
        lineColor
    );
    renderer->drawDebugLine(
        Vector3(logoCenter.x + 95 * scale, baseY + 5 * scale, 0),
        Vector3(logoCenter.x + 125 * scale, baseY + 5 * scale, 0),
        lineColor
    );
    renderer->drawDebugLine(
        Vector3(logoCenter.x + 125 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x + 125 * scale, baseY - 30 * scale, 0),
        lineColor
    );
    renderer->drawDebugLine(
        Vector3(logoCenter.x + 95 * scale, baseY + 5 * scale, 0),
        Vector3(logoCenter.x + 125 * scale, baseY - 30 * scale, 0),
        lineColor
    );

    // Y - Coming together at bottom
    renderer->drawDebugLine(
        Vector3(logoCenter.x + 155 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x + 170 * scale, baseY - 5 * scale, 0),
        lineColor
    );
    renderer->drawDebugLine(
        Vector3(logoCenter.x + 185 * scale, baseY + 30 * scale, 0),
        Vector3(logoCenter.x + 170 * scale, baseY - 5 * scale, 0),
        lineColor
    );
    renderer->drawDebugBox(
        Vector3(logoCenter.x + 170 * scale, baseY - 30 * scale, 0),
        Vector3(2 * scale, 25 * scale, 0.1f),
        lineColor
    );

    // Draw "ENGINE" below "FOUNDRY"
    Vector2 enginePos(logoCenter.x, logoCenter.y - 80 * scale);

    // E
    renderer->drawDebugLine(Vector3(enginePos.x - 140 * scale, enginePos.y - 15 * scale, 0),
                          Vector3(enginePos.x - 140 * scale, enginePos.y + 15 * scale, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x - 140 * scale, enginePos.y + 15 * scale, 0),
                          Vector3(enginePos.x - 110 * scale, enginePos.y + 15 * scale, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x - 140 * scale, enginePos.y, 0),
                          Vector3(enginePos.x - 115 * scale, enginePos.y, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x - 140 * scale, enginePos.y - 15 * scale, 0),
                          Vector3(enginePos.x - 110 * scale, enginePos.y - 15 * scale, 0), lineColor);

    // N
    renderer->drawDebugLine(Vector3(enginePos.x - 90 * scale, enginePos.y - 15 * scale, 0),
                          Vector3(enginePos.x - 90 * scale, enginePos.y + 15 * scale, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x - 90 * scale, enginePos.y + 15 * scale, 0),
                          Vector3(enginePos.x - 55 * scale, enginePos.y - 15 * scale, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x - 55 * scale, enginePos.y - 15 * scale, 0),
                          Vector3(enginePos.x - 55 * scale, enginePos.y + 15 * scale, 0), lineColor);

    // G
    Vector3 gCenter(enginePos.x - 25 * scale, enginePos.y, 0);
    for (int i = 3; i < 29; ++i) { // Leave opening at bottom
        float angle1 = (i / 32.0f) * 2 * 3.14159f - 3.14159f/4;
        float angle2 = ((i + 1) / 32.0f) * 2 * 3.14159f - 3.14159f/4;
        renderer->drawDebugLine(
            Vector3(gCenter.x + cos(angle1) * oRadius, gCenter.y + sin(angle1) * oRadius, 0),
            Vector3(gCenter.x + cos(angle2) * oRadius, gCenter.y + sin(angle2) * oRadius, 0),
            lineColor
        );
    }
    renderer->drawDebugLine(Vector3(gCenter.x, gCenter.y, 0),
                          Vector3(gCenter.x + 15 * scale, gCenter.y, 0), lineColor);

    // I
    renderer->drawDebugLine(Vector3(enginePos.x + 15 * scale, enginePos.y - 15 * scale, 0),
                          Vector3(enginePos.x + 15 * scale, enginePos.y + 15 * scale, 0), lineColor);

    // N
    renderer->drawDebugLine(Vector3(enginePos.x + 35 * scale, enginePos.y - 15 * scale, 0),
                          Vector3(enginePos.x + 35 * scale, enginePos.y + 15 * scale, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x + 35 * scale, enginePos.y + 15 * scale, 0),
                          Vector3(enginePos.x + 70 * scale, enginePos.y - 15 * scale, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x + 70 * scale, enginePos.y - 15 * scale, 0),
                          Vector3(enginePos.x + 70 * scale, enginePos.y + 15 * scale, 0), lineColor);

    // E
    renderer->drawDebugLine(Vector3(enginePos.x + 90 * scale, enginePos.y - 15 * scale, 0),
                          Vector3(enginePos.x + 90 * scale, enginePos.y + 15 * scale, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x + 90 * scale, enginePos.y + 15 * scale, 0),
                          Vector3(enginePos.x + 120 * scale, enginePos.y + 15 * scale, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x + 90 * scale, enginePos.y, 0),
                          Vector3(enginePos.x + 115 * scale, enginePos.y, 0), lineColor);
    renderer->drawDebugLine(Vector3(enginePos.x + 90 * scale, enginePos.y - 15 * scale, 0),
                          Vector3(enginePos.x + 120 * scale, enginePos.y - 15 * scale, 0), lineColor);

    // Draw glowing effect around logo using debug boxes
    Vector3 glowColor(0.4f, 0.6f, 1.0f); // Electric blue glow
    float glowSize = 8 * scale;
    for (float offset = 1; offset <= glowSize; offset += 2) {
        renderer->drawDebugBox(
            Vector3(logoCenter.x, logoCenter.y, -offset),
            Vector3(500 * scale + offset, 200 * scale + offset, offset * 2),
            Vector3(glowColor.x, glowColor.y, glowColor.z)
        );
    }

    // ASCII art fallback for console/debugging
    std::cout << "[FoundryEngine] Displaying Foundry Engine Logo" << std::endl;
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
