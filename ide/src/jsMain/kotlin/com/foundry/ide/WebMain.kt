package com.foundry.ide

import kotlinx.browser.document
import kotlinx.browser.window
import kotlinx.coroutines.MainScope
import kotlinx.coroutines.launch
import kotlinx.html.*
import kotlinx.html.dom.create
import kotlinx.html.js.onClickFunction
import org.w3c.dom.HTMLButtonElement
import org.w3c.dom.HTMLDivElement
import org.w3c.dom.HTMLInputElement
import org.w3c.dom.asList

/**
 * Web main entry point for Foundry IDE
 * Creates a web-based interface using Kotlin/JS and HTML DOM
 */
fun main() {
    // Initialize the IDE application
    initializeIde()

    // Create the web UI
    createWebInterface()

    // Set up global error handling
    window.onerror = { message, source, lineno, colno, error ->
        console.error("JavaScript error: $message at $source:$lineno:$colno")
        false
    }

    window.onload = {
        console.log("Foundry IDE Web loaded successfully")
    }
}

private fun createWebInterface() {
    val root = document.getElementById("foundry-ide-root")
        ?: document.body?.appendChild(document.create.div {
            id = "foundry-ide-root"
            style = """
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
                background: #1e1e1e;
                color: white;
                height: 100vh;
                margin: 0;
                padding: 0;
                overflow: hidden;
            """.trimIndent()
        })

    if (root != null) {
        // Create main layout
        root.appendChild(createMainLayout())
    }
}

private fun createMainLayout(): HTMLDivElement {
    return document.create.div {
        id = "main-layout"
        style = """
            display: flex;
            flex-direction: column;
            height: 100vh;
        """.trimIndent()

        // Top toolbar
        div {
            id = "toolbar"
            style = """
                background: #2d2d2d;
                padding: 8px 16px;
                border-bottom: 1px solid #404040;
                display: flex;
                align-items: center;
                gap: 8px;
            """.trimIndent()

            // Menu buttons
            button {
                + "File"
                style = """
                    background: #4a90e2;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    cursor: pointer;
                """.trimIndent()
                onClickFunction = { showFileMenu() }
            }

            button {
                + "Edit"
                style = """
                    background: #4a90e2;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    cursor: pointer;
                """.trimIndent()
                onClickFunction = { showEditMenu() }
            }

            button {
                + "View"
                style = """
                    background: #4a90e2;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    cursor: pointer;
                """.trimIndent()
                onClickFunction = { showViewMenu() }
            }

            button {
                + "Build"
                style = """
                    background: #4a90e2;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    cursor: pointer;
                """.trimIndent()
                onClickFunction = { showBuildMenu() }
            }

            button {
                + "Run"
                style = """
                    background: #28a745;
                    color: white;
                    border: none;
                    padding: 8px 16px;
                    border-radius: 4px;
                    cursor: pointer;
                """.trimIndent()
                onClickFunction = { ideApp.runProject("web") }
            }
        }

        // Main content area
        div {
            id = "content-area"
            style = """
                flex: 1;
                display: flex;
                overflow: hidden;
            """.trimIndent()

            // Sidebar
            div {
                id = "sidebar"
                style = """
                    width: 250px;
                    background: #252526;
                    border-right: 1px solid #404040;
                    padding: 8px;
                    overflow-y: auto;
                """.trimIndent()

                h3 {
                    + "Project"
                    style = """
                        color: #4a90e2;
                        margin: 0 0 8px 0;
                        font-size: 14px;
                    """.trimIndent()
                }

                // Project tree (placeholder)
                div {
                    id = "project-tree"
                    + "No project loaded"
                    style = """
                        color: #888;
                        font-style: italic;
                        padding: 8px;
                    """.trimIndent()
                }
            }

            // Main editor area
            div {
                id = "editor-area"
                style = """
                    flex: 1;
                    background: #1e1e1e;
                    position: relative;
                """.trimIndent()

                // Welcome screen
                div {
                    id = "welcome-screen"
                    style = """
                        position: absolute;
                        top: 0;
                        left: 0;
                        right: 0;
                        bottom: 0;
                        display: flex;
                        flex-direction: column;
                        align-items: center;
                        justify-content: center;
                        background: #1e1e1e;
                    """.trimIndent()

                    h1 {
                        + "Welcome to Foundry IDE"
                        style = """
                            color: white;
                            font-size: 32px;
                            margin-bottom: 16px;
                        """.trimIndent()
                    }

                    p {
                        + "Create powerful games and applications with the Foundry Game Engine"
                        style = """
                            color: #ccc;
                            font-size: 16px;
                            margin-bottom: 32px;
                        """.trimIndent()
                    }

                    button {
                        + "New Project"
                        style = """
                            background: #4a90e2;
                            color: white;
                            border: none;
                            padding: 16px 32px;
                            font-size: 16px;
                            border-radius: 8px;
                            cursor: pointer;
                            margin-right: 16px;
                        """.trimIndent()
                        onClickFunction = { showNewProjectDialog() }
                    }

                    button {
                        + "Open Project"
                        style = """
                            background: #28a745;
                            color: white;
                            border: none;
                            padding: 16px 32px;
                            font-size: 16px;
                            border-radius: 8px;
                            cursor: pointer;
                        """.trimIndent()
                        onClickFunction = { showOpenProjectDialog() }
                    }
                }

                // Editor views (initially hidden)
                div {
                    id = "world-editor"
                    style = "display: none; position: absolute; top: 0; left: 0; right: 0; bottom: 0;"
                    + "World Editor - Coming Soon"
                }

                div {
                    id = "code-editor"
                    style = "display: none; position: absolute; top: 0; left: 0; right: 0; bottom: 0;"
                    + "Code Editor - Coming Soon"
                }
            }
        }

        // Status bar
        div {
            id = "status-bar"
            style = """
                background: #007acc;
                color: white;
                padding: 4px 16px;
                font-size: 12px;
            """.trimIndent()
            + "Ready"
        }
    }
}

// Dialog management
private var activeDialog: HTMLDivElement? = null

private fun showFileMenu() {
    showDropdownMenu("File", listOf(
        "New Project" to { showNewProjectDialog() },
        "Open Project" to { showOpenProjectDialog() },
        "Save Project" to { ideApp.saveProject() },
        "Import Asset" to { showImportAssetDialog() },
        "Export Project" to { showExportProjectDialog() }
    ))
}

private fun showEditMenu() {
    showDropdownMenu("Edit", listOf(
        "Undo" to { /* TODO */ },
        "Redo" to { /* TODO */ },
        "Preferences" to { showPreferencesDialog() }
    ))
}

private fun showViewMenu() {
    showDropdownMenu("View", listOf(
        "World Editor" to { switchToView("world-editor") },
        "Code Editor" to { switchToView("code-editor") },
        "Asset Browser" to { switchToView("asset-browser") }
    ))
}

private fun showBuildMenu() {
    showDropdownMenu("Build", listOf(
        "Build Web" to { buildForTarget("web") },
        "Build Desktop" to { buildForTarget("desktop") },
        "Build Mobile" to { buildForTarget("mobile") }
    ))
}

private fun showDropdownMenu(title: String, items: List<Pair<String, () -> Unit>>) {
    // Remove existing dropdown
    document.getElementById("dropdown-menu")?.remove()

    val dropdown = document.create.div {
        id = "dropdown-menu"
        style = """
            position: absolute;
            top: 40px;
            left: 0;
            background: #2d2d2d;
            border: 1px solid #404040;
            border-radius: 4px;
            z-index: 1000;
            min-width: 150px;
        """.trimIndent()

        items.forEach { (label, action) ->
            div {
                +label
                style = """
                    padding: 8px 16px;
                    cursor: pointer;
                    hover { background: #404040; }
                """.trimIndent()
                onClickFunction = {
                    action()
                    dropdown.remove()
                }
            }
        }
    }

    document.getElementById("toolbar")?.appendChild(dropdown)

    // Close dropdown when clicking outside
    document.onclick = { event ->
        if (event.target != dropdown && dropdown.contains(event.target as? org.w3c.dom.Node) == false) {
            dropdown.remove()
            document.onclick = null
        }
    }
}

private fun switchToView(viewId: String) {
    // Hide all views
    arrayOf("welcome-screen", "world-editor", "code-editor").forEach { id ->
        document.getElementById(id)?.style?.display = "none"
    }

    // Show selected view
    document.getElementById(viewId)?.style?.display = "block"
}

private fun buildForTarget(target: String) {
    MainScope().launch {
        val result = ideApp.buildProject(target)
        val statusBar = document.getElementById("status-bar")
        if (result.success) {
            statusBar?.textContent = "Build successful for $target"
        } else {
            statusBar?.textContent = "Build failed: ${result.errors.joinToString(", ")}"
        }
    }
}

// Dialog functions
private fun showNewProjectDialog() {
    showDialog("New Project", """
        <div style="padding: 16px;">
            <label>Project Name:</label><br>
            <input type="text" id="project-name" placeholder="My Game Project" style="width: 100%; margin: 8px 0; padding: 8px;"><br>
            <label>Location:</label><br>
            <input type="text" id="project-location" placeholder="./projects/" style="width: 100%; margin: 8px 0; padding: 8px;"><br><br>
            <button onclick="createProject()" style="background: #4a90e2; color: white; border: none; padding: 8px 16px; margin-right: 8px;">Create</button>
            <button onclick="closeDialog()">Cancel</button>
        </div>
    """.trimIndent())
}

private fun showOpenProjectDialog() {
    // In a real implementation, this would show a file browser
    // For now, just show a simple input
    showDialog("Open Project", """
        <div style="padding: 16px;">
            <label>Project Path:</label><br>
            <input type="text" id="project-path" placeholder="./my-project/" style="width: 100%; margin: 8px 0; padding: 8px;"><br><br>
            <button onclick="openProject()">Open</button>
            <button onclick="closeDialog()">Cancel</button>
        </div>
    """.trimIndent())
}

private fun showImportAssetDialog() {
    showDialog("Import Asset", "Asset import dialog - Coming Soon")
}

private fun showExportProjectDialog() {
    showDialog("Export Project", "Project export dialog - Coming Soon")
}

private fun showPreferencesDialog() {
    showDialog("Preferences", "Preferences dialog - Coming Soon")
}

private fun showDialog(title: String, content: String) {
    // Remove existing dialog
    activeDialog?.remove()

    activeDialog = document.create.div {
        id = "modal-dialog"
        style = """
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0, 0, 0, 0.5);
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 2000;
        """.trimIndent()

        div {
            style = """
                background: #2d2d2d;
                border-radius: 8px;
                min-width: 400px;
                max-width: 90vw;
                border: 1px solid #404040;
            """.trimIndent()

            div {
                style = """
                    background: #1e1e1e;
                    padding: 16px;
                    border-bottom: 1px solid #404040;
                    font-weight: bold;
                    font-size: 18px;
                """.trimIndent()
                + title
            }

            div {
                style = "padding: 0;" // Content will be set via innerHTML
                id = "dialog-content"
            }
        }
    }

    document.body?.appendChild(activeDialog!!)
    document.getElementById("dialog-content")?.innerHTML = content

    // Add global functions for dialog interaction
    window.asDynamic().createProject = {
        val name = (document.getElementById("project-name") as? HTMLInputElement)?.value ?: ""
        val location = (document.getElementById("project-location") as? HTMLInputElement)?.value ?: "./"
        val path = if (location.endsWith("/")) "$location$name" else "$location/$name"

        MainScope().launch {
            ideApp.createProject(name, path)
            closeDialog()
        }
    }

    window.asDynamic().openProject = {
        val path = (document.getElementById("project-path") as? HTMLInputElement)?.value ?: ""
        MainScope().launch {
            ideApp.loadProject(path)
            closeDialog()
        }
    }

    window.asDynamic().closeDialog = { closeDialog() }
}

private fun closeDialog() {
    activeDialog?.remove()
    activeDialog = null
}

// Console helpers
external fun console.log(message: Any?)
external fun console.error(message: Any?)
external fun console.warn(message: Any?)

// Window event handlers
external var window.onload: (() -> Unit)?
external var window.onerror: ((Any?, String?, Int?, Int?, Any?) -> Boolean)?
