package ide.plugins.builtin.themes

import com.foundry.ide.*
import com.foundry.ide.managers.*
import androidx.compose.ui.graphics.Color
import androidx.compose.material.MaterialTheme
import androidx.compose.material.darkColors

/**
 * Built-in Dark Theme Plugin for Foundry IDE
 */
class DarkThemePlugin : IdePlugin {
    override val metadata = PluginMetadata(
        id = "foundry.theme.dark",
        name = "Foundry Dark Theme",
        version = "1.0.0",
        description = "Professional dark theme for Foundry IDE",
        author = "Foundry Team",
        license = "MIT",
        mainClass = "ide.plugins.builtin.themes.DarkThemePlugin",
        enabled = true,
        minIdeVersion = "1.0.0",
        tags = listOf("theme", "dark", "ui", "builtin")
    )

    override val context = PluginContext(
        pluginId = metadata.id,
        ideVersion = "1.0.0",
        workspacePath = "",
        configPath = "",
        tempPath = "",
        permissions = listOf("ui.theme", "settings.read", "settings.write")
    )

    override fun initialize(ideApp: IdeApplication): Boolean {
        try {
            // Register theme contribution
            registerTheme()

            // Apply theme if it's the current theme
            val currentTheme = settingsManager.getSettings().theme
            if (currentTheme == "dark") {
                applyTheme()
            }

            return true
        } catch (e: Exception) {
            println("Failed to initialize Dark Theme plugin: ${e.message}")
            return false
        }
    }

    override fun shutdown(): Boolean {
        try {
            // Cleanup theme resources
            return true
        } catch (e: Exception) {
            println("Failed to shutdown Dark Theme plugin: ${e.message}")
            return false
        }
    }

    override fun getMenuContributions(): List<MenuContribution> = emptyList()

    override fun getToolbarContributions(): List<ToolbarContribution> = emptyList()

    override fun getViewContributions(): List<ViewContribution> = emptyList()

    override fun getCommandContributions(): List<CommandContribution> = listOf(
        CommandContribution(
            id = "foundry.theme.dark.apply",
            title = "Apply Dark Theme",
            category = "Theme",
            icon = "dark_mode"
        )
    )

    override fun getKeybindingContributions(): List<KeybindingContribution> = emptyList()

    override fun getSettingsContributions(): List<SettingsContribution> = listOf(
        SettingsContribution(
            id = "foundry.theme.dark",
            title = "Dark Theme Settings",
            properties = listOf(
                SettingProperty(
                    id = "accentColor",
                    title = "Accent Color",
                    description = "Primary accent color for the dark theme",
                    type = SettingType.STRING,
                    defaultValue = "#007ACC"
                ),
                SettingProperty(
                    id = "backgroundIntensity",
                    title = "Background Intensity",
                    description = "Darkness level of background colors",
                    type = SettingType.NUMBER,
                    defaultValue = 0.9,
                    minimum = 0.1,
                    maximum = 1.0
                )
            )
        )
    )

    override fun getThemeContributions(): List<ThemeContribution> = listOf(
        ThemeContribution(
            id = "foundry-dark",
            label = "Foundry Dark",
            uiTheme = "vs-dark",
            path = "themes/dark.json"
        )
    )

    override fun getLanguageContributions(): List<LanguageContribution> = emptyList()

    override fun getSnippetContributions(): List<SnippetContribution> = emptyList()

    override fun onProjectLoaded(project: ProjectInfo?) {}

    override fun onProjectClosed() {}

    override fun onBuildStarted(target: String) {}

    override fun onBuildCompleted(result: BuildResult) {}

    override fun onIdeStartup() {}

    override fun onIdeShutdown() {}

    override fun onWorkspaceChanged(path: String) {}

    override fun onSettingsChanged(settings: Map<String, Any>) {
        // Reapply theme if settings changed
        val theme = settings["theme"] as? String
        if (theme == "dark") {
            applyTheme()
        }
    }

    override fun handleCommand(commandId: String, parameters: Map<String, Any>): Any? {
        return when (commandId) {
            "foundry.theme.dark.apply" -> {
                settingsManager.updateSetting { it.copy(theme = "dark") }
                applyTheme()
                true
            }
            else -> null
        }
    }

    override fun validatePermissions(): List<String> = context.permissions

    override fun getExtensionPoints(): List<ExtensionPoint> = emptyList()

    override fun extend(extensionPoint: String, extension: Any): Boolean = false

    private fun registerTheme() {
        // Register the theme with the IDE
        val themeData = createThemeData()
        // This would integrate with the IDE's theme system
    }

    private fun applyTheme() {
        // Apply the dark theme to the UI
        val themeColors = darkColors(
            primary = Color(0xFF007ACC),
            primaryVariant = Color(0xFF005A9E),
            secondary = Color(0xFF4CAF50),
            background = Color(0xFF1E1E1E),
            surface = Color(0xFF2D2D2D),
            onPrimary = Color.White,
            onSecondary = Color.White,
            onBackground = Color.White,
            onSurface = Color.White
        )

        // Apply theme to MaterialTheme
        // This would be handled by the IDE's theme system
    }

    private fun createThemeData(): Map<String, Any> {
        return mapOf(
            "name" to "Foundry Dark",
            "type" to "dark",
            "colors" to mapOf(
                "editor.background" to "#1e1e1e",
                "editor.foreground" to "#d4d4d4",
                "editor.lineHighlightBackground" to "#2d2d30",
                "editor.selectionBackground" to "#264f78",
                "editorCursor.foreground" to "#ffffff",
                "editorWhitespace.foreground" to "#404040",
                "editorLineNumber.foreground" to "#858585",
                "editorLineNumber.activeForeground" to "#c6c6c6",
                "editorWidget.background" to "#252526",
                "editorWidget.border" to "#454545",
                "editorSuggestWidget.background" to "#252526",
                "editorSuggestWidget.border" to "#454545",
                "editorGroupHeader.tabsBackground" to "#2d2d30",
                "tab.activeBackground" to "#1e1e1e",
                "tab.inactiveBackground" to "#2d2d30",
                "tab.activeForeground" to "#ffffff",
                "tab.inactiveForeground" to "#cccccc",
                "panel.background" to "#252526",
                "panel.border" to "#3e3e42",
                "statusBar.background" to "#007acc",
                "statusBar.foreground" to "#ffffff",
                "titleBar.activeBackground" to "#3c3c3c",
                "titleBar.activeForeground" to "#cccccc",
                "menu.background" to "#2d2d2d",
                "menu.foreground" to "#cccccc",
                "menu.selectionBackground" to "#094771",
                "menu.selectionForeground" to "#ffffff",
                "input.background" to "#3c3c3c",
                "input.foreground" to "#cccccc",
                "input.border" to "#3e3e42",
                "button.background" to "#4a90e2",
                "button.foreground" to "#ffffff",
                "dropdown.background" to "#3c3c3c",
                "dropdown.foreground" to "#cccccc",
                "dropdown.border" to "#3e3e42",
                "scrollbar.shadow" to "#000000",
                "scrollbarSlider.background" to "#797979",
                "scrollbarSlider.hoverBackground" to "#646464",
                "scrollbarSlider.activeBackground" to "#bfbfbf"
            ),
            "tokenColors" to listOf(
                mapOf(
                    "scope" to listOf("comment", "punctuation.definition.comment"),
                    "settings" to mapOf("foreground" to "#6A9955")
                ),
                mapOf(
                    "scope" to listOf("keyword", "storage.type", "storage.modifier"),
                    "settings" to mapOf("foreground" to "#569CD6")
                ),
                mapOf(
                    "scope" to listOf("string", "punctuation.definition.string"),
                    "settings" to mapOf("foreground" to "#CE9178")
                ),
                mapOf(
                    "scope" to listOf("number", "constant.numeric"),
                    "settings" to mapOf("foreground" to "#B5CEA8")
                ),
                mapOf(
                    "scope" to listOf("entity.name.type", "entity.name.class"),
                    "settings" to mapOf("foreground" to "#4EC9B0")
                ),
                mapOf(
                    "scope" to listOf("variable", "entity.name.variable"),
                    "settings" to mapOf("foreground" to "#9CDCFE")
                ),
                mapOf(
                    "scope" to listOf("function", "entity.name.function"),
                    "settings" to mapOf("foreground" to "#DCDCAA")
                )
            )
        )
    }
}