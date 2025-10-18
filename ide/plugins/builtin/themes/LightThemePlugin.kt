package ide.plugins.builtin.themes

import com.foundry.ide.*
import com.foundry.ide.managers.*
import androidx.compose.ui.graphics.Color
import androidx.compose.material.MaterialTheme
import androidx.compose.material.lightColors

/**
 * Built-in Light Theme Plugin for Foundry IDE
 */
class LightThemePlugin : IdePlugin {
    override val metadata = PluginMetadata(
        id = "foundry.theme.light",
        name = "Foundry Light Theme",
        version = "1.0.0",
        description = "Clean light theme for Foundry IDE",
        author = "Foundry Team",
        license = "MIT",
        mainClass = "ide.plugins.builtin.themes.LightThemePlugin",
        enabled = true,
        minIdeVersion = "1.0.0",
        tags = listOf("theme", "light", "ui", "builtin")
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
            registerTheme()

            val currentTheme = settingsManager.getSettings().theme
            if (currentTheme == "light") {
                applyTheme()
            }

            return true
        } catch (e: Exception) {
            println("Failed to initialize Light Theme plugin: ${e.message}")
            return false
        }
    }

    override fun shutdown(): Boolean = true

    override fun getMenuContributions(): List<MenuContribution> = emptyList()

    override fun getToolbarContributions(): List<ToolbarContribution> = emptyList()

    override fun getViewContributions(): List<ViewContribution> = emptyList()

    override fun getCommandContributions(): List<CommandContribution> = listOf(
        CommandContribution(
            id = "foundry.theme.light.apply",
            title = "Apply Light Theme",
            category = "Theme",
            icon = "light_mode"
        )
    )

    override fun getKeybindingContributions(): List<KeybindingContribution> = emptyList()

    override fun getSettingsContributions(): List<SettingsContribution> = listOf(
        SettingsContribution(
            id = "foundry.theme.light",
            title = "Light Theme Settings",
            properties = listOf(
                SettingProperty(
                    id = "accentColor",
                    title = "Accent Color",
                    description = "Primary accent color for the light theme",
                    type = SettingType.STRING,
                    defaultValue = "#007ACC"
                ),
                SettingProperty(
                    id = "contrast",
                    title = "Contrast Level",
                    description = "Contrast level for better readability",
                    type = SettingType.NUMBER,
                    defaultValue = 1.0,
                    minimum = 0.8,
                    maximum = 1.5
                )
            )
        )
    )

    override fun getThemeContributions(): List<ThemeContribution> = listOf(
        ThemeContribution(
            id = "foundry-light",
            label = "Foundry Light",
            uiTheme = "vs",
            path = "themes/light.json"
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
        val theme = settings["theme"] as? String
        if (theme == "light") {
            applyTheme()
        }
    }

    override fun handleCommand(commandId: String, parameters: Map<String, Any>): Any? {
        return when (commandId) {
            "foundry.theme.light.apply" -> {
                settingsManager.updateSetting { it.copy(theme = "light") }
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
        val themeData = createThemeData()
        // Register with IDE theme system
    }

    private fun applyTheme() {
        val themeColors = lightColors(
            primary = Color(0xFF007ACC),
            primaryVariant = Color(0xFF005A9E),
            secondary = Color(0xFF4CAF50),
            background = Color(0xFFFFFFFF),
            surface = Color(0xFFF5F5F5),
            onPrimary = Color.White,
            onSecondary = Color.White,
            onBackground = Color.Black,
            onSurface = Color.Black
        )

        // Apply to MaterialTheme
    }

    private fun createThemeData(): Map<String, Any> {
        return mapOf(
            "name" to "Foundry Light",
            "type" to "light",
            "colors" to mapOf(
                "editor.background" to "#ffffff",
                "editor.foreground" to "#000000",
                "editor.lineHighlightBackground" to "#f0f0f0",
                "editor.selectionBackground" to "#add6ff",
                "editorCursor.foreground" to "#000000",
                "editorWhitespace.foreground" to "#d3d3d3",
                "editorLineNumber.foreground" to "#237893",
                "editorLineNumber.activeForeground" to "#0b216f",
                "editorWidget.background" to "#f3f3f3",
                "editorWidget.border" to "#c8c8c8",
                "editorSuggestWidget.background" to "#f3f3f3",
                "editorSuggestWidget.border" to "#c8c8c8",
                "editorGroupHeader.tabsBackground" to "#f3f3f3",
                "tab.activeBackground" to "#ffffff",
                "tab.inactiveBackground" to "#ececec",
                "tab.activeForeground" to "#000000",
                "tab.inactiveForeground" to "#616161",
                "panel.background" to "#f3f3f3",
                "panel.border" to "#c8c8c8",
                "statusBar.background" to "#007acc",
                "statusBar.foreground" to "#ffffff",
                "titleBar.activeBackground" to "#dddddd",
                "titleBar.activeForeground" to "#3c3c3c",
                "menu.background" to "#ffffff",
                "menu.foreground" to "#616161",
                "menu.selectionBackground" to "#007acc",
                "menu.selectionForeground" to "#ffffff",
                "input.background" to "#ffffff",
                "input.foreground" to "#616161",
                "input.border" to "#c8c8c8",
                "button.background" to "#007acc",
                "button.foreground" to "#ffffff",
                "dropdown.background" to "#ffffff",
                "dropdown.foreground" to "#616161",
                "dropdown.border" to "#c8c8c8",
                "scrollbar.shadow" to "#000000",
                "scrollbarSlider.background" to "#c8c8c8",
                "scrollbarSlider.hoverBackground" to "#a8a8a8",
                "scrollbarSlider.activeBackground" to "#616161"
            ),
            "tokenColors" to listOf(
                mapOf(
                    "scope" to listOf("comment", "punctuation.definition.comment"),
                    "settings" to mapOf("foreground" to "#008000")
                ),
                mapOf(
                    "scope" to listOf("keyword", "storage.type", "storage.modifier"),
                    "settings" to mapOf("foreground" to "#0000ff")
                ),
                mapOf(
                    "scope" to listOf("string", "punctuation.definition.string"),
                    "settings" to mapOf("foreground" to "#a31515")
                ),
                mapOf(
                    "scope" to listOf("number", "constant.numeric"),
                    "settings" to mapOf("foreground" to "#09885a")
                ),
                mapOf(
                    "scope" to listOf("entity.name.type", "entity.name.class"),
                    "settings" to mapOf("foreground" to "#267f99")
                ),
                mapOf(
                    "scope" to listOf("variable", "entity.name.variable"),
                    "settings" to mapOf("foreground" to "#001080")
                ),
                mapOf(
                    "scope" to listOf("function", "entity.name.function"),
                    "settings" to mapOf("foreground" to "#795e26")
                )
            )
        )
    }
}