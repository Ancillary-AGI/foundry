# Foundry IDE Plugin Development Guide

This guide provides comprehensive instructions for developing plugins for the Foundry IDE.

## Table of Contents

1. [Plugin Architecture](#plugin-architecture)
2. [Creating a Basic Plugin](#creating-a-basic-plugin)
3. [Plugin Structure](#plugin-structure)
4. [Plugin Metadata](#plugin-metadata)
5. [Extension Points](#extension-points)
6. [Contributing Extensions](#contributing-extensions)
7. [Plugin Lifecycle](#plugin-lifecycle)
8. [Best Practices](#best-practices)
9. [Publishing Plugins](#publishing-plugins)
10. [Troubleshooting](#troubleshooting)

## Plugin Architecture

Foundry IDE plugins are based on a modular architecture that allows extending the IDE's functionality through well-defined extension points. Plugins are written in Kotlin and packaged as JAR files.

### Key Components

- **Plugin Manager**: Manages plugin loading, lifecycle, and extension points
- **Extension Points**: Interfaces where plugins can contribute functionality
- **Contributions**: Specific implementations that extend the IDE
- **Plugin Context**: Provides access to IDE services and resources

## Creating a Basic Plugin

### Step 1: Set Up Project Structure

Create a new Kotlin project with the following structure:

```
my-plugin/
├── src/main/kotlin/
│   └── com/example/myplugin/
│       ├── MyPlugin.kt
│       └── plugin.json
├── build.gradle.kts
└── README.md
```

### Step 2: Define Plugin Metadata

Create `plugin.json` in your resources directory:

```json
{
  "id": "com.example.myplugin",
  "name": "My Awesome Plugin",
  "version": "1.0.0",
  "description": "A plugin that adds awesome features to Foundry IDE",
  "author": "Your Name",
  "license": "MIT",
  "homepage": "https://github.com/yourname/my-plugin",
  "repository": "https://github.com/yourname/my-plugin.git",
  "dependencies": [],
  "permissions": ["filesystem.read"],
  "mainClass": "com.example.myplugin.MyPlugin",
  "minIdeVersion": "1.0.0",
  "maxIdeVersion": null,
  "tags": ["productivity", "editor"],
  "apiVersion": "1.0"
}
```

### Step 3: Implement the Plugin Class

```kotlin
package com.example.myplugin

import com.foundry.ide.managers.*

class MyPlugin : IdePlugin {

    override val metadata: PluginMetadata = PluginMetadata(
        id = "com.example.myplugin",
        name = "My Awesome Plugin",
        version = "1.0.0",
        description = "A plugin that adds awesome features",
        author = "Your Name",
        license = "MIT",
        mainClass = "com.example.myplugin.MyPlugin",
        permissions = listOf("filesystem.read")
    )

    override lateinit var context: PluginContext

    override fun initialize(ideApp: IdeApplication): Boolean {
        println("MyPlugin initialized!")
        return true
    }

    override fun shutdown(): Boolean {
        println("MyPlugin shutdown!")
        return true
    }

    // Implement other required methods...
    override fun getMenuContributions(): List<MenuContribution> = emptyList()
    override fun getToolbarContributions(): List<ToolbarContribution> = emptyList()
    override fun getViewContributions(): List<ViewContribution> = emptyList()
    override fun getCommandContributions(): List<CommandContribution> = emptyList()
    override fun getKeybindingContributions(): List<KeybindingContribution> = emptyList()
    override fun getSettingsContributions(): List<SettingsContribution> = emptyList()
    override fun getThemeContributions(): List<ThemeContribution> = emptyList()
    override fun getLanguageContributions(): List<LanguageContribution> = emptyList()
    override fun getSnippetContributions(): List<SnippetContribution> = emptyList()

    override fun onProjectLoaded(project: ProjectInfo?) {}
    override fun onProjectClosed() {}
    override fun onBuildStarted(target: String) {}
    override fun onBuildCompleted(result: BuildResult) {}
    override fun onIdeStartup() {}
    override fun onIdeShutdown() {}
    override fun onWorkspaceChanged(path: String) {}
    override fun onSettingsChanged(settings: Map<String, Any>) {}

    override fun handleCommand(commandId: String, parameters: Map<String, Any>): Any? {
        return when (commandId) {
            "myplugin.hello" -> {
                println("Hello from MyPlugin!")
                "Hello World!"
            }
            else -> null
        }
    }

    override fun validatePermissions(): List<String> = emptyList()
    override fun getExtensionPoints(): List<ExtensionPoint> = emptyList()
    override fun extend(extensionPoint: String, extension: Any): Boolean = false
}
```

### Step 4: Build Configuration

Create `build.gradle.kts`:

```kotlin
plugins {
    kotlin("jvm") version "1.8.0"
}

group = "com.example"
version = "1.0.0"

repositories {
    mavenCentral()
}

dependencies {
    compileOnly("com.foundry:ide-api:1.0.0")
    implementation(kotlin("stdlib-jdk8"))
}
```

### Step 5: Build and Package

```bash
./gradlew build
```

The built JAR will be in `build/libs/`.

## Plugin Structure

### Required Files

- `plugin.json`: Plugin metadata and configuration
- Main class implementing `IdePlugin` interface
- Any additional resources (icons, templates, etc.)

### Optional Files

- `README.md`: Plugin documentation
- `CHANGELOG.md`: Version history
- `LICENSE`: Plugin license
- Additional JARs for dependencies

## Plugin Metadata

The `plugin.json` file contains essential information about your plugin:

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| id | String | Yes | Unique plugin identifier |
| name | String | Yes | Display name |
| version | String | Yes | Semantic version |
| description | String | Yes | Plugin description |
| author | String | Yes | Plugin author |
| license | String | Yes | License type |
| mainClass | String | Yes | Main plugin class |
| minIdeVersion | String | Yes | Minimum IDE version |
| apiVersion | String | Yes | API version |

## Extension Points

Foundry IDE provides several extension points for plugins:

### Commands
Add custom commands to the IDE:

```kotlin
override fun getCommandContributions(): List<CommandContribution> {
    return listOf(
        CommandContribution(
            id = "myplugin.hello",
            title = "Say Hello",
            category = "My Plugin",
            icon = "hello-icon.png"
        )
    )
}
```

### Menus
Add menu items:

```kotlin
override fun getMenuContributions(): List<MenuContribution> {
    return listOf(
        MenuContribution(
            menu = "Tools",
            items = listOf(
                MenuItem(
                    id = "myplugin.hello",
                    label = "Say Hello",
                    action = "myplugin.hello"
                )
            )
        )
    )
}
```

### Views
Add custom UI panels:

```kotlin
override fun getViewContributions(): List<ViewContribution> {
    return listOf(
        ViewContribution(
            id = "myplugin.panel",
            name = "My Panel",
            category = "My Plugin",
            component = "MyPluginPanel"
        )
    )
}
```

### Languages
Add support for new programming languages:

```kotlin
override fun getLanguageContributions(): List<LanguageContribution> {
    return listOf(
        LanguageContribution(
            id = "mylang",
            aliases = listOf("mylang", "ml"),
            extensions = listOf(".mylang", ".ml"),
            configuration = "mylang-config.json"
        )
    )
}
```

### Themes
Add custom UI themes:

```kotlin
override fun getThemeContributions(): List<ThemeContribution> {
    return listOf(
        ThemeContribution(
            id = "mytheme",
            label = "My Theme",
            uiTheme = "vs-dark",
            path = "themes/mytheme.json"
        )
    )
}
```

## Contributing Extensions

### Extending Existing Plugins

Plugins can extend other plugins through extension points:

```kotlin
override fun extend(extensionPoint: String, extension: Any): Boolean {
    return when (extensionPoint) {
        "myplugin.extensions" -> {
            // Handle extension
            true
        }
        else -> false
    }
}
```

### Creating Extension Points

Plugins can define their own extension points:

```kotlin
override fun getExtensionPoints(): List<ExtensionPoint> {
    return listOf(
        ExtensionPoint(
            id = "myplugin.extensions",
            name = "My Plugin Extensions",
            description = "Extensions for My Plugin"
        )
    )
}
```

## Plugin Lifecycle

Plugins follow a strict lifecycle:

1. **Discovery**: Plugin JAR is found in plugins directory
2. **Validation**: Plugin metadata and JAR are validated
3. **Loading**: Plugin class is loaded with custom class loader
4. **Initialization**: Plugin's `initialize()` method is called
5. **Active**: Plugin is fully loaded and contributing to the IDE
6. **Shutdown**: Plugin's `shutdown()` method is called

### Lifecycle Methods

- `initialize()`: Called when plugin is loaded
- `shutdown()`: Called when plugin is unloaded
- `onProjectLoaded()`: Called when a project is opened
- `onIdeStartup()`: Called when IDE starts
- `onIdeShutdown()`: Called when IDE shuts down

## Best Practices

### Code Quality

1. **Error Handling**: Always handle exceptions gracefully
2. **Resource Management**: Clean up resources in `shutdown()`
3. **Thread Safety**: Be aware of threading concerns
4. **Performance**: Don't block the UI thread

### User Experience

1. **Clear Naming**: Use descriptive names for commands and UI elements
2. **Documentation**: Provide clear documentation and tooltips
3. **Feedback**: Give users feedback on actions
4. **Consistency**: Follow IDE conventions

### Security

1. **Permission Requests**: Only request necessary permissions
2. **Input Validation**: Validate all inputs
3. **Safe APIs**: Use safe APIs when available
4. **Code Review**: Have security reviews for published plugins

## Publishing Plugins

### Plugin Marketplace

1. **Create Account**: Sign up for a marketplace account
2. **Package Plugin**: Build and package your plugin
3. **Submit for Review**: Upload plugin for review
4. **Publish**: Plugin becomes available in marketplace

### Manual Installation

Users can install plugins manually by:

1. Downloading the JAR file
2. Placing it in the `ide/plugins/` directory
3. Restarting the IDE

## Troubleshooting

### Common Issues

**Plugin Not Loading**
- Check `plugin.json` for syntax errors
- Verify main class exists and implements `IdePlugin`
- Check IDE version compatibility

**Extension Not Working**
- Verify extension point IDs are correct
- Check that contributions are properly formatted
- Ensure plugin is enabled

**Performance Issues**
- Use profiling tools to identify bottlenecks
- Avoid blocking operations on UI thread
- Implement lazy loading where possible

### Debug Logging

Enable debug logging to troubleshoot issues:

```kotlin
// In your plugin
println("Debug: MyPlugin action performed")
```

### Getting Help

- Check the [plugin development forum](https://forum.foundry-ide.com/plugins)
- Review the [API documentation](https://docs.foundry-ide.com/api)
- File issues on the [GitHub repository](https://github.com/foundry-ide/foundry)

## Example Plugins

### Hello World Plugin

A minimal plugin that adds a "Hello World" command:

```kotlin
class HelloWorldPlugin : IdePlugin {
    // ... metadata and other methods ...

    override fun getCommandContributions(): List<CommandContribution> {
        return listOf(
            CommandContribution(
                id = "helloworld.greet",
                title = "Say Hello",
                category = "Hello World"
            )
        )
    }

    override fun handleCommand(commandId: String, parameters: Map<String, Any>): Any? {
        return when (commandId) {
            "helloworld.greet" -> {
                println("Hello, World!")
                "Hello, World!"
            }
            else -> null
        }
    }
}
```

### Language Support Plugin

A plugin that adds syntax highlighting for a custom language:

```kotlin
class MyLanguagePlugin : IdePlugin {
    // ... metadata and other methods ...

    override fun getLanguageContributions(): List<LanguageContribution> {
        return listOf(
            LanguageContribution(
                id = "mylang",
                aliases = listOf("mylang"),
                extensions = listOf(".mylang"),
                configuration = "language-config.json"
            )
        )
    }
}
```

This guide covers the basics of plugin development for Foundry IDE. For more advanced topics, refer to the API documentation and example plugins in the repository.