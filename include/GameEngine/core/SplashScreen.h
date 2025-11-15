/**
 * @file SplashScreen.h
 * @brief Foundry Engine splash screen system for branding games built with the platform
 * @author FoundryEngine Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include "../core/System.h"
#include <functional>
#include <string>
#include <vector>
#include <chrono>

namespace FoundryEngine {

// Forward declarations
class Texture;

/**
 * @struct SplashScreenConfig
 * @brief Configuration for the Foundry splash screen
 */
struct SplashScreenConfig {
    bool enabled = true;                    ///< Enable/disable splash screen
    bool showLogo = true;                   ///< Show Foundry logo
    bool showVersion = true;                ///< Show version info
    bool showLoadingProgress = true;        ///< Show loading progress bar
    float duration = 3.0f;                  ///< Total splash screen duration (seconds)
    float fadeInTime = 0.5f;                ///< Fade in duration
    float fadeOutTime = 0.5f;               ///< Fade out duration
    std::string customMessage;              ///< Optional custom message
    std::string backgroundColor = "#1a1a1a"; ///< Background color (hex)
    std::string textColor = "#ffffff";       ///< Text color (hex)
    int screenWidth = 1920;                 ///< Screen width for scaling
    int screenHeight = 1080;                ///< Screen height for scaling
};

/**
 * @class SplashScreen
 * @brief Manages the display of the Foundry Engine splash screen
 *
 * This system ensures that games built with Foundry Engine display
 * the official Foundry branding at startup, similar to Unity/Unreal Engine.
 */
class SplashScreen : public System {
public:
    /**
     * @brief Constructor
     */
    SplashScreen();

    /**
     * @brief Destructor
     */
    ~SplashScreen();

    /**
     * @brief Initialize the splash screen system
     * @param config Configuration for the splash screen
     * @return true if initialization successful
     */
    bool initialize(const SplashScreenConfig& config = SplashScreenConfig{}) override;

    /**
     * @brief Shutdown the splash screen system
     */
    void shutdown() override;

    /**
     * @brief Update the splash screen animation
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime) override;

    /**
     * @brief Render the splash screen
     */
    void render();

    /**
     * @brief Check if splash screen is currently active
     * @return true if splash screen is being displayed
     */
    bool isActive() const;

    /**
     * @brief Get current splash screen progress (0.0 to 1.0)
     * @return Progress value
     */
    float getProgress() const;

    /**
     * @brief Set custom loading progress for the progress bar
     * @param progress Progress value (0.0 to 1.0)
     */
    void setLoadingProgress(float progress);

    /**
     * @brief Skip the splash screen (for development/testing)
     */
    void skip();

    /**
     * @brief Set a callback function to be called when splash screen ends
     * @param callback Function to call when splash screen completes
     */
    void setCompletionCallback(std::function<void()> callback);

    /**
     * @brief Set custom message to display on splash screen
     * @param message Custom message text
     */
    void setCustomMessage(const std::string& message);

    /**
     * @brief Get the current configuration
     * @return Current splash screen configuration
     */
    const SplashScreenConfig& getConfig() const { return config_; }

private:
    /**
     * @brief Load Foundry logo texture
     * @return true if logo loaded successfully
     */
    bool loadLogoTexture();

    /**
     * @brief Create default splash screen textures if logo not found
     */
    void createDefaultBranding();

    /**
     * @brief Render the Foundry logo
     */
    void renderLogo();

    /**
     * @brief Render version information
     */
    void renderVersionInfo();

    /**
     * @brief Render loading progress bar
     */
    void renderProgressBar();

    /**
     * @brief Render custom message
     */
    void renderCustomMessage();

    /**
     * @brief Apply fade in/out effects
     */
    void applyFadeEffect();

    /**
     * @brief Calculate logo scaling based on screen size
     * @return Scale factor for logo
     */
    float calculateLogoScale() const;

    /**
     * @brief Get the official Foundry Engine version string
     * @return Version string
     */
    std::string getEngineVersion() const;

    // Internal state
    SplashScreenConfig config_;
    bool active_ = false;
    bool initialized_ = false;
    float totalElapsedTime_ = 0.0f;
    float loadingProgress_ = 0.0f;
    float opacity_ = 0.0f;
    Texture* logoTexture_ = nullptr;
    std::function<void()> completionCallback_;
    std::string customMessage_;
    std::chrono::steady_clock::time_point startTime_;
};

} // namespace FoundryEngine
