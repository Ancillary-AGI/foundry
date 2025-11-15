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

    applyFadeEffect();

    // Clear background with configured color
    // This would be done through the renderer system

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
    if (!logoTexture_) {
        // Render default Foundry text logo
        std::cout << "[FoundryEngine] Rendering Foundry Engine Logo" << std::endl;
        return;
    }

    // Renderer integration would go here
    // Scale logo appropriately based on screen size
    float scale = calculateLogoScale();

    // Center logo on screen
    Vector2 screenCenter(config_.screenWidth / 2.0f, config_.screenHeight / 2.0f);
    Vector2 logoSize(400 * scale, 200 * scale); // Default logo size
    Vector2 logoPosition = screenCenter - logoSize * 0.5f;
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
