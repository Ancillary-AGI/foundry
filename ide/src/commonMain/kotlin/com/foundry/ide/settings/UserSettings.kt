package com.foundry.ide.settings

import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File
import androidx.compose.runtime.*
import androidx.compose.ui.graphics.Color

/**
 * Comprehensive user settings system for Foundry IDE
 */
@Serializable
data class UserSettings(
    // General Settings
    val theme: String = "dark",
    val language: String = "en",
    val autoSave: Boolean = true,
    val autoSaveDelay: Int = 1000, // milliseconds

    // Editor Settings
    val editor: EditorSettings = EditorSettings(),

    // Workspace Settings
    val workspace: WorkspaceSettings = WorkspaceSettings(),

    // Plugin Settings
    val plugins: PluginSettings = PluginSettings(),

    // Build Settings
    val build: BuildSettings = BuildSettings(),

    // Marketplace Settings
    val marketplace: MarketplaceSettings = MarketplaceSettings(),

    // Advanced Settings
    val advanced: AdvancedSettings = AdvancedSettings()
)

@Serializable
data class EditorSettings(
    val fontFamily: String = "JetBrains Mono",
    val fontSize: Int = 14,
    val tabSize: Int = 4,
    val insertSpaces: Boolean = true,
    val wordWrap: Boolean = false,
    val minimap: Boolean = true,
    val lineNumbers: Boolean = true,
    val folding: Boolean = true,
    val bracketMatching: Boolean = true,
    val autoComplete: Boolean = true,
    val formatOnSave: Boolean = true,
    val formatOnPaste: Boolean = true,
    val showWhitespace: Boolean = false,
    val renderWhitespace: String = "selection", // "all", "selection", "none"
    val cursorStyle: String = "line", // "line", "block", "underline"
    val cursorBlinking: String = "blink", // "blink", "solid", "phase"
    val smoothScrolling: Boolean = true,
    val mouseWheelZoom: Boolean = true,
    val multiCursorModifier: String = "alt", // "alt", "ctrl"
    val accessibilitySupport: Boolean = true,
    val rulers: List<Int> = listOf(80, 120),
    val glyphMargin: Boolean = true,
    val useTabStops: Boolean = false,
    val detectIndentation: Boolean = true,
    val trimAutoWhitespace: Boolean = true,
    val scrollBeyondLastLine: Boolean = true,
    val automaticLayout: Boolean = true,
    val padding: EditorPadding = EditorPadding(),
    val suggest: SuggestSettings = SuggestSettings()
)

@Serializable
data class EditorPadding(
    val top: Int = 0,
    val bottom: Int = 0,
    val left: Int = 0,
    val right: Int = 0
)

@Serializable
data class SuggestSettings(
    val enabled: Boolean = true,
    val showKeywords: Boolean = true,
    val showSnippets: Boolean = true,
    val showIcons: Boolean = true,
    val maxVisibleSuggestions: Int = 12,
    val filterGraceful: Boolean = true,
    val localityBonus: Boolean = true,
    val shareSuggestSelections: Boolean = true,
    val showInlineDetails: Boolean = true,
    val showStatusBar: Boolean = true
)

@Serializable
data class WorkspaceSettings(
    val name: String = "",
    val path: String = "",
    val trustUntrustedFiles: Boolean = false,
    val enablePreviewFeatures: Boolean = false,
    val hotExit: String = "onExit", // "onExit", "onExitAndWindowClose", "off"
    val useGlobalStorage: Boolean = true,
    val startupEditor: String = "welcomePage", // "welcomePage", "newUntitledFile", "none"
    val restoreWindows: String = "all", // "all", "folders", "one", "none"
    val restoreViewState: Boolean = true,
    val zoomLevel: Float = 0f,
    val window: WindowSettings = WindowSettings(),
    val search: SearchSettings = SearchSettings(),
    val files: FilesSettings = FilesSettings(),
    val git: GitSettings = GitSettings()
)

@Serializable
data class WindowSettings(
    val menuBarVisibility: String = "classic", // "classic", "visible", "toggle", "hidden"
    val openFilesInNewWindow: String = "off", // "on", "off", "default"
    val openFoldersInNewWindow: String = "default", // "on", "off", "default"
    val reopenFolders: String = "all", // "all", "folders", "one", "none"
    val restoreFullscreen: Boolean = false,
    val zoomLevel: Float = 0f,
    val titleBarStyle: String = "native", // "native", "custom"
    val windowControls: String = "native" // "native", "left"
)

@Serializable
data class SearchSettings(
    val exclude: Map<String, Boolean> = mapOf(
        "**/node_modules" to true,
        "**/bower_components" to true,
        "**/*.code-search" to true
    ),
    val useParentIgnoreFiles: Boolean = true,
    val useGlobalIgnoreFiles: Boolean = true,
    val followSymlinks: Boolean = true,
    val smartCase: Boolean = false,
    val globalFindClipboard: Boolean = false,
    val location: String = "sidebar", // "sidebar", "panel"
    val useReplacePreview: Boolean = true,
    val showLineNumbers: Boolean = false,
    val usePCRE2: Boolean = false
)

@Serializable
data class FilesSettings(
    val exclude: Map<String, Boolean> = mapOf(
        "**/.git" to true,
        "**/.svn" to true,
        "**/.hg" to true,
        "**/CVS" to true,
        "**/.DS_Store" to true,
        "**/Thumbs.db" to true
    ),
    val associations: Map<String, String> = emptyMap(),
    val encoding: String = "utf8",
    val autoGuessEncoding: Boolean = false,
    val eol: String = "auto", // "auto", "\n", "\r\n"
    val trimTrailingWhitespace: Boolean = false,
    val insertFinalNewline: Boolean = false,
    val trimFinalNewlines: Boolean = false,
    val hotExit: String = "onExit",
    val useExperimentalFileWatcher: Boolean = false,
    val watcherExclude: Map<String, Boolean> = emptyMap(),
    val simpleDialog: Map<String, Boolean> = emptyMap()
)

@Serializable
data class GitSettings(
    val enabled: Boolean = true,
    val autofetch: Boolean = true,
    val autofetchPeriod: Int = 180, // seconds
    val confirmSync: Boolean = true,
    val countBadge: String = "all", // "all", "tracked", "off"
    val checkoutType: String = "all", // "all", "local", "tags", "remote"
    val branchProtection: List<String> = emptyList(),
    val branchSortOrder: String = "committerdate", // "alphabetically", "committerdate"
    val branchValidation: Boolean = true,
    val inputValidation: Boolean = true,
    val inputValidationLength: Int = 50,
    val inputValidationSubjectLength: Int = 50,
    val showInlineOpenFileAction: Boolean = true,
    val showPushSuccessNotification: Boolean = false
)

@Serializable
data class PluginSettings(
    val autoUpdate: Boolean = true,
    val autoCheckUpdates: Boolean = true,
    val showRecommendations: Boolean = true,
    val ignoreRecommendations: List<String> = emptyList(),
    val enableTelemetry: Boolean = false,
    val experimental: Boolean = false
)

@Serializable
data class BuildSettings(
    val preferredTarget: String = "web",
    val optimizationLevel: Int = 2,
    val sourceMap: Boolean = true,
    val minify: Boolean = true,
    val targetESVersion: String = "es2020",
    val enableWasm: Boolean = true,
    val enableThreads: Boolean = false,
    val enableSIMD: Boolean = true,
    val customBuildFlags: List<String> = emptyList()
)

@Serializable
data class MarketplaceSettings(
    val trustedPublishers: List<String> = emptyList(),
    val autoDownloadDependencies: Boolean = true,
    val preferredSort: String = "installs", // "installs", "rating", "name", "updated"
    val showPreReleaseVersions: Boolean = false,
    val usePreReleaseVersions: Boolean = false
)

@Serializable
data class AdvancedSettings(
    val developerMode: Boolean = false,
    val enableLogging: Boolean = true,
    val logLevel: String = "info", // "error", "warn", "info", "debug", "trace"
    val enableProfiling: Boolean = false,
    val enableCrashReporting: Boolean = true,
    val enableUpdateChannel: String = "stable", // "stable", "insiders", "none"
    val proxySupport: String = "off", // "off", "on", "fallback"
    val proxyUrl: String = "",
    val security: SecuritySettings = SecuritySettings(),
    val telemetry: TelemetrySettings = TelemetrySettings()
)

@Serializable
data class SecuritySettings(
    val workspaceTrust: WorkspaceTrustSettings = WorkspaceTrustSettings(),
    val pluginValidation: Boolean = true,
    val allowUnsignedPlugins: Boolean = false,
    val restrictMode: Boolean = false
)

@Serializable
data class WorkspaceTrustSettings(
    val enabled: Boolean = true,
    val emptyWindow: Boolean = true,
    val untrustedFiles: String = "prompt", // "open", "prompt", "newWindow"
    val startupPrompt: Boolean = true,
    val banner: Boolean = true
)

@Serializable
data class TelemetrySettings(
    val enableTelemetry: Boolean = false,
    val enableCrashReporter: Boolean = true,
    val enableExperiments: Boolean = false,
    val machineId: String = ""
)

/**
 * Settings manager for handling user preferences
 */
class SettingsManager {
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
        encodeDefaults = true
    }

    private var currentSettings = UserSettings()
    private val settingsFile = getSettingsFile()

    init {
        loadSettings()
    }

    fun getSettings(): UserSettings = currentSettings

    fun updateSettings(newSettings: UserSettings) {
        currentSettings = newSettings
        saveSettings()
        notifySettingsChanged()
    }

    fun updateSetting(updater: (UserSettings) -> UserSettings) {
        currentSettings = updater(currentSettings)
        saveSettings()
        notifySettingsChanged()
    }

    fun getSetting(path: String): Any? {
        return getNestedValue(currentSettings, path.split("."))
    }

    fun setSetting(path: String, value: Any) {
        val newSettings = setNestedValue(currentSettings, path.split("."), value)
        if (newSettings != null) {
            currentSettings = newSettings
            saveSettings()
            notifySettingsChanged()
        }
    }

    fun resetToDefaults() {
        currentSettings = UserSettings()
        saveSettings()
        notifySettingsChanged()
    }

    fun exportSettings(): String {
        return json.encodeToString(UserSettings.serializer(), currentSettings)
    }

    fun importSettings(jsonString: String) {
        try {
            val imported = json.decodeFromString(UserSettings.serializer(), jsonString)
            currentSettings = imported
            saveSettings()
            notifySettingsChanged()
        } catch (e: Exception) {
            throw IllegalArgumentException("Invalid settings format: ${e.message}")
        }
    }

    private fun loadSettings() {
        try {
            if (settingsFile.exists()) {
                val content = settingsFile.readText()
                currentSettings = json.decodeFromString(UserSettings.serializer(), content)
            }
        } catch (e: Exception) {
            // If loading fails, use defaults and create backup
            val backupFile = File(settingsFile.parent, "settings.json.backup")
            if (settingsFile.exists()) {
                settingsFile.copyTo(backupFile, overwrite = true)
            }
            currentSettings = UserSettings()
            saveSettings()
        }
    }

    private fun saveSettings() {
        try {
            settingsFile.parentFile?.mkdirs()
            val content = json.encodeToString(UserSettings.serializer(), currentSettings)
            settingsFile.writeText(content)
        } catch (e: Exception) {
            throw RuntimeException("Failed to save settings: ${e.message}")
        }
    }

    private fun notifySettingsChanged() {
        // Notify all interested components about settings changes
        // This would integrate with the event system
    }

    private fun getSettingsFile(): File {
        val userHome = System.getProperty("user.home")
        return File(userHome, ".foundry-ide/settings.json")
    }

    private fun getNestedValue(obj: Any, path: List<String>): Any? {
        if (path.isEmpty()) return obj

        return when (obj) {
            is UserSettings -> when (path[0]) {
                "theme" -> obj.theme
                "language" -> obj.language
                "autoSave" -> obj.autoSave
                "autoSaveDelay" -> obj.autoSaveDelay
                "editor" -> getNestedValue(obj.editor, path.drop(1))
                "workspace" -> getNestedValue(obj.workspace, path.drop(1))
                "plugins" -> getNestedValue(obj.plugins, path.drop(1))
                "build" -> getNestedValue(obj.build, path.drop(1))
                "marketplace" -> getNestedValue(obj.marketplace, path.drop(1))
                "advanced" -> getNestedValue(obj.advanced, path.drop(1))
                else -> null
            }
            is EditorSettings -> when (path[0]) {
                "fontFamily" -> obj.fontFamily
                "fontSize" -> obj.fontSize
                "tabSize" -> obj.tabSize
                "insertSpaces" -> obj.insertSpaces
                "wordWrap" -> obj.wordWrap
                "minimap" -> obj.minimap
                "lineNumbers" -> obj.lineNumbers
                "folding" -> obj.folding
                "bracketMatching" -> obj.bracketMatching
                "autoComplete" -> obj.autoComplete
                "formatOnSave" -> obj.formatOnSave
                "formatOnPaste" -> obj.formatOnPaste
                "showWhitespace" -> obj.showWhitespace
                "renderWhitespace" -> obj.renderWhitespace
                "cursorStyle" -> obj.cursorStyle
                "cursorBlinking" -> obj.cursorBlinking
                "smoothScrolling" -> obj.smoothScrolling
                "mouseWheelZoom" -> obj.mouseWheelZoom
                "multiCursorModifier" -> obj.multiCursorModifier
                "accessibilitySupport" -> obj.accessibilitySupport
                "rulers" -> obj.rulers
                "glyphMargin" -> obj.glyphMargin
                "useTabStops" -> obj.useTabStops
                "detectIndentation" -> obj.detectIndentation
                "trimAutoWhitespace" -> obj.trimAutoWhitespace
                "scrollBeyondLastLine" -> obj.scrollBeyondLastLine
                "automaticLayout" -> obj.automaticLayout
                "padding" -> getNestedValue(obj.padding, path.drop(1))
                "suggest" -> getNestedValue(obj.suggest, path.drop(1))
                else -> null
            }
            // Add more cases for other settings objects...
            else -> null
        }
    }

    private fun setNestedValue(obj: Any, path: List<String>, value: Any): UserSettings? {
        // This would be a complex implementation to set nested values
        // For now, return null to indicate not implemented
        return null
    }
}

// Global settings manager instance
val settingsManager = SettingsManager()