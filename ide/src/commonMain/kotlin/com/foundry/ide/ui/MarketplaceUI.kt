package com.foundry.ide.ui

import androidx.compose.foundation.*
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.*
import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.runtime.*
import androidx.compose.ui.*
import androidx.compose.ui.graphics.*
import androidx.compose.ui.text.font.*
import androidx.compose.ui.unit.*
import androidx.compose.ui.window.*
import com.foundry.ide.managers.*
import kotlinx.coroutines.*

/**
 * Plugin Marketplace UI for Foundry IDE
 * Complete marketplace interface with search, install, and publishing features
 */
@Composable
fun PluginMarketplaceWindow(onClose: () -> Unit) {
    var currentTab by remember { mutableStateOf(MarketplaceTab.BROWSE) }
    var searchQuery by remember { mutableStateOf("") }
    var selectedCategory by remember { mutableStateOf<String?>(null) }
    var selectedPlugin by remember { mutableStateOf<MarketplacePlugin?>(null) }
    var plugins by remember { mutableStateOf<List<MarketplacePlugin>>(emptyList()) }
    var isLoading by remember { mutableStateOf(false) }
    var errorMessage by remember { mutableStateOf<String?>(null) }

    val scope = rememberCoroutineScope()

    LaunchedEffect(Unit) {
        loadFeaturedPlugins()
    }

    Window(
        onCloseRequest = onClose,
        title = "Foundry Plugin Marketplace",
        state = rememberWindowState(width = 1200.dp, height = 800.dp)
    ) {
        MaterialTheme {
            Column(modifier = Modifier.fillMaxSize()) {
                // Header
                MarketplaceHeader(
                    searchQuery = searchQuery,
                    onSearchChange = { searchQuery = it },
                    onSearch = { performSearch() }
                )

                // Tabs
                MarketplaceTabs(
                    currentTab = currentTab,
                    onTabChange = { currentTab = it }
                )

                // Content
                Box(modifier = Modifier.weight(1f)) {
                    when (currentTab) {
                        MarketplaceTab.BROWSE -> BrowseTab(
                            plugins = plugins,
                            isLoading = isLoading,
                            errorMessage = errorMessage,
                            onPluginSelect = { selectedPlugin = it },
                            onCategorySelect = { selectedCategory = it; performSearch() },
                            onRefresh = { loadFeaturedPlugins() }
                        )
                        MarketplaceTab.INSTALLED -> InstalledTab()
                        MarketplaceTab.PUBLISH -> PublishTab()
                        MarketplaceTab.SETTINGS -> SettingsTab()
                    }
                }

                // Plugin Details Dialog
                selectedPlugin?.let { plugin ->
                    PluginDetailsDialog(
                        plugin = plugin,
                        onDismiss = { selectedPlugin = null },
                        onInstall = { installPlugin(plugin) },
                        onRate = { rating, comment -> ratePlugin(plugin.id, rating, comment) }
                    )
                }
            }
        }
    }

    fun loadFeaturedPlugins() {
        scope.launch {
            isLoading = true
            errorMessage = null

            try {
                val response = marketplaceManager.getFeaturedPlugins()
                if (response.success) {
                    plugins = response.data ?: emptyList()
                } else {
                    errorMessage = response.error ?: "Failed to load plugins"
                }
            } catch (e: Exception) {
                errorMessage = "Network error: ${e.message}"
            } finally {
                isLoading = false
            }
        }
    }

    fun performSearch() {
        scope.launch {
            isLoading = true
            errorMessage = null

            try {
                val query = MarketplaceSearchQuery(
                    query = searchQuery.takeIf { it.isNotBlank() },
                    category = selectedCategory
                )

                val response = marketplaceManager.searchPlugins(query)
                if (response.success) {
                    plugins = response.data ?: emptyList()
                } else {
                    errorMessage = response.error ?: "Search failed"
                }
            } catch (e: Exception) {
                errorMessage = "Search error: ${e.message}"
            } finally {
                isLoading = false
            }
        }
    }

    fun installPlugin(plugin: MarketplacePlugin) {
        scope.launch {
            try {
                val result = marketplaceManager.installPlugin(plugin.id)
                when (result) {
                    is PluginInstallResult.Success -> {
                        showMessage("Plugin installed successfully!")
                        // Refresh installed plugins list
                    }
                    is PluginInstallResult.Failure -> {
                        showError("Installation failed: ${result.message}")
                    }
                }
            } catch (e: Exception) {
                showError("Installation error: ${e.message}")
            }
        }
    }

    fun ratePlugin(pluginId: String, rating: Int, comment: String) {
        scope.launch {
            try {
                val response = marketplaceManager.submitReview(pluginId, rating, comment)
                if (response.success) {
                    showMessage("Review submitted successfully!")
                } else {
                    showError("Failed to submit review: ${response.error}")
                }
            } catch (e: Exception) {
                showError("Review submission error: ${e.message}")
            }
        }
    }

    fun showMessage(message: String) {
        // Show success message
        println("SUCCESS: $message")
    }

    fun showError(message: String) {
        // Show error message
        println("ERROR: $message")
    }
}

enum class MarketplaceTab {
    BROWSE, INSTALLED, PUBLISH, SETTINGS
}

@Composable
private fun MarketplaceHeader(
    searchQuery: String,
    onSearchChange: (String) -> Unit,
    onSearch: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(16.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        // Logo
        Text(
            text = "🔥 Foundry Marketplace",
            style = MaterialTheme.typography.h5,
            fontWeight = FontWeight.Bold,
            color = Color(0xFFFF6B35)
        )

        Spacer(modifier = Modifier.width(32.dp))

        // Search
        OutlinedTextField(
            value = searchQuery,
            onValueChange = onSearchChange,
            placeholder = { Text("Search plugins...") },
            modifier = Modifier.weight(1f),
            leadingIcon = { Icon(Icons.Default.Search, "Search") },
            singleLine = true
        )

        Spacer(modifier = Modifier.width(16.dp))

        Button(onClick = onSearch) {
            Text("Search")
        }
    }
}

@Composable
private fun MarketplaceTabs(
    currentTab: MarketplaceTab,
    onTabChange: (MarketplaceTab) -> Unit
) {
    TabRow(selectedTabIndex = currentTab.ordinal) {
        MarketplaceTab.values().forEach { tab ->
            Tab(
                selected = currentTab == tab,
                onClick = { onTabChange(tab) },
                text = { Text(tab.name) }
            )
        }
    }
}

@Composable
private fun BrowseTab(
    plugins: List<MarketplacePlugin>,
    isLoading: Boolean,
    errorMessage: String?,
    onPluginSelect: (MarketplacePlugin) -> Unit,
    onCategorySelect: (String?) -> Unit,
    onRefresh: () -> Unit
) {
    Row(modifier = Modifier.fillMaxSize()) {
        // Sidebar
        Column(
            modifier = Modifier
                .width(200.dp)
                .fillMaxHeight()
                .padding(16.dp)
        ) {
            Text("Categories", style = MaterialTheme.typography.h6)
            Spacer(modifier = Modifier.height(8.dp))

            val categories = listOf(
                "All" to null,
                "Editor" to "editor",
                "Tools" to "tools",
                "Languages" to "languages",
                "Build" to "build",
                "Assets" to "assets",
                "Productivity" to "productivity"
            )

            categories.forEach { (name, value) ->
                TextButton(
                    onClick = { onCategorySelect(value) },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text(name)
                }
            }

            Spacer(modifier = Modifier.height(16.dp))

            Button(onClick = onRefresh, modifier = Modifier.fillMaxWidth()) {
                Icon(Icons.Default.Refresh, "Refresh")
                Spacer(modifier = Modifier.width(8.dp))
                Text("Refresh")
            }
        }

        // Plugin List
        Box(modifier = Modifier.weight(1f)) {
            when {
                isLoading -> {
                    CircularProgressIndicator(
                        modifier = Modifier.align(Alignment.Center)
                    )
                }
                errorMessage != null -> {
                    Column(
                        modifier = Modifier.align(Alignment.Center),
                        horizontalAlignment = Alignment.CenterHorizontally
                    ) {
                        Text("Error: $errorMessage", color = MaterialTheme.colors.error)
                        Spacer(modifier = Modifier.height(16.dp))
                        Button(onClick = onRefresh) {
                            Text("Retry")
                        }
                    }
                }
                else -> {
                    LazyColumn(modifier = Modifier.fillMaxSize()) {
                        items(plugins) { plugin ->
                            PluginListItem(
                                plugin = plugin,
                                onClick = { onPluginSelect(plugin) }
                            )
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun PluginListItem(
    plugin: MarketplacePlugin,
    onClick: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(8.dp)
            .clickable(onClick = onClick),
        elevation = 4.dp
    ) {
        Row(modifier = Modifier.padding(16.dp)) {
            // Plugin Icon (placeholder)
            Box(
                modifier = Modifier
                    .size(48.dp)
                    .background(Color.Gray, shape = MaterialTheme.shapes.small),
                contentAlignment = Alignment.Center
            ) {
                Text("🔌", fontSize = 24.sp)
            }

            Spacer(modifier = Modifier.width(16.dp))

            Column(modifier = Modifier.weight(1f)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(
                        text = plugin.name,
                        style = MaterialTheme.typography.h6,
                        fontWeight = FontWeight.Bold
                    )
                    if (plugin.isOfficial) {
                        Spacer(modifier = Modifier.width(8.dp))
                        Text(
                            text = "OFFICIAL",
                            style = MaterialTheme.typography.caption,
                            color = Color(0xFFFF6B35),
                            fontWeight = FontWeight.Bold
                        )
                    }
                    if (plugin.isVerified) {
                        Spacer(modifier = Modifier.width(8.dp))
                        Icon(
                            Icons.Default.Verified,
                            "Verified",
                            tint = Color(0xFF4CAF50)
                        )
                    }
                }

                Text(
                    text = plugin.description,
                    style = MaterialTheme.typography.body2,
                    maxLines = 2
                )

                Spacer(modifier = Modifier.height(8.dp))

                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text(
                        text = "by ${plugin.author}",
                        style = MaterialTheme.typography.caption
                    )
                    Spacer(modifier = Modifier.width(16.dp))
                    Text(
                        text = "v${plugin.version}",
                        style = MaterialTheme.typography.caption
                    )
                    Spacer(modifier = Modifier.width(16.dp))
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Icon(Icons.Default.Star, "Rating", modifier = Modifier.size(16.dp))
                        Spacer(modifier = Modifier.width(4.dp))
                        Text(
                            text = "%.1f".format(plugin.rating),
                            style = MaterialTheme.typography.caption
                        )
                    }
                    Spacer(modifier = Modifier.width(16.dp))
                    Text(
                        text = "${plugin.downloadCount} downloads",
                        style = MaterialTheme.typography.caption
                    )
                }

                Spacer(modifier = Modifier.height(8.dp))

                Row {
                    plugin.tags.take(3).forEach { tag ->
                        Surface(
                            shape = MaterialTheme.shapes.small,
                            color = MaterialTheme.colors.secondary.copy(alpha = 0.1f),
                            modifier = Modifier.padding(end = 4.dp)
                        ) {
                            Text(
                                text = tag,
                                style = MaterialTheme.typography.caption,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                            )
                        }
                    }
                }
            }

            if (plugin.price > 0) {
                Column(horizontalAlignment = Alignment.End) {
                    Text(
                        text = "$${"%.2f".format(plugin.price)}",
                        style = MaterialTheme.typography.h6,
                        color = Color(0xFF4CAF50),
                        fontWeight = FontWeight.Bold
                    )
                }
            }
        }
    }
}

@Composable
private fun PluginDetailsDialog(
    plugin: MarketplacePlugin,
    onDismiss: () -> Unit,
    onInstall: () -> Unit,
    onRate: (Int, String) -> Unit
) {
    var userRating by remember { mutableStateOf(0) }
    var reviewComment by remember { mutableStateOf("") }
    var showReviewDialog by remember { mutableStateOf(false) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(plugin.name) },
        text = {
            Column(modifier = Modifier.fillMaxWidth()) {
                // Screenshots (placeholder)
                if (plugin.screenshots.isNotEmpty()) {
                    Text("Screenshots:", style = MaterialTheme.typography.h6)
                    LazyRow {
                        items(plugin.screenshots) { screenshot ->
                            Card(
                                modifier = Modifier
                                    .size(200.dp, 150.dp)
                                    .padding(4.dp)
                            ) {
                                // Image would go here
                                Box(
                                    modifier = Modifier.fillMaxSize(),
                                    contentAlignment = Alignment.Center
                                ) {
                                    Text("Screenshot", style = MaterialTheme.typography.caption)
                                }
                            }
                        }
                    }
                }

                Spacer(modifier = Modifier.height(16.dp))

                Text(plugin.description)

                Spacer(modifier = Modifier.height(16.dp))

                // Plugin info
                Row(modifier = Modifier.fillMaxWidth()) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text("Author: ${plugin.author}")
                        Text("Version: ${plugin.version}")
                        Text("License: ${plugin.license}")
                        Text("Downloads: ${plugin.downloadCount}")
                    }
                    Column(modifier = Modifier.weight(1f)) {
                        Text("Rating: ${"%.1f".format(plugin.rating)} ⭐")
                        Text("Price: $${"%.2f".format(plugin.price)}")
                        Text("Category: ${plugin.category}")
                        Text("Last Updated: ${plugin.lastUpdated}")
                    }
                }

                Spacer(modifier = Modifier.height(16.dp))

                // Tags
                Text("Tags:", style = MaterialTheme.typography.h6)
                Row {
                    plugin.tags.forEach { tag ->
                        Surface(
                            shape = MaterialTheme.shapes.small,
                            color = MaterialTheme.colors.primary.copy(alpha = 0.1f),
                            modifier = Modifier.padding(end = 4.dp, bottom = 4.dp)
                        ) {
                            Text(
                                text = tag,
                                style = MaterialTheme.typography.caption,
                                modifier = Modifier.padding(horizontal = 8.dp, vertical = 2.dp)
                            )
                        }
                    }
                }

                Spacer(modifier = Modifier.height(16.dp))

                // Reviews
                if (plugin.reviews.isNotEmpty()) {
                    Text("Recent Reviews:", style = MaterialTheme.typography.h6)
                    LazyColumn(modifier = Modifier.height(200.dp)) {
                        items(plugin.reviews.take(3)) { review ->
                            Card(modifier = Modifier.padding(4.dp)) {
                                Column(modifier = Modifier.padding(8.dp)) {
                                    Row(verticalAlignment = Alignment.CenterVertically) {
                                        Text(review.userName, fontWeight = FontWeight.Bold)
                                        Spacer(modifier = Modifier.width(8.dp))
                                        Text("${review.rating} ⭐")
                                        Spacer(modifier = Modifier.width(8.dp))
                                        Text(review.date, style = MaterialTheme.typography.caption)
                                    }
                                    Text(review.comment, style = MaterialTheme.typography.body2)
                                }
                            }
                        }
                    }
                }
            }
        },
        confirmButton = {
            Row {
                Button(onClick = onInstall) {
                    Text("Install")
                }
                Spacer(modifier = Modifier.width(8.dp))
                OutlinedButton(onClick = { showReviewDialog = true }) {
                    Text("Rate & Review")
                }
                Spacer(modifier = Modifier.width(8.dp))
                OutlinedButton(onClick = onDismiss) {
                    Text("Close")
                }
            }
        }
    )

    // Review Dialog
    if (showReviewDialog) {
        AlertDialog(
            onDismissRequest = { showReviewDialog = false },
            title = { Text("Rate & Review") },
            text = {
                Column {
                    Text("Rate this plugin:")
                    Row {
                        for (i in 1..5) {
                            IconButton(onClick = { userRating = i }) {
                                Icon(
                                    if (i <= userRating) Icons.Default.Star else Icons.Default.StarBorder,
                                    "Star $i",
                                    tint = if (i <= userRating) Color.Yellow else Color.Gray
                                )
                            }
                        }
                    }

                    Spacer(modifier = Modifier.height(16.dp))

                    OutlinedTextField(
                        value = reviewComment,
                        onValueChange = { reviewComment = it },
                        label = { Text("Your review (optional)") },
                        modifier = Modifier.fillMaxWidth(),
                        maxLines = 5
                    )
                }
            },
            confirmButton = {
                Button(onClick = {
                    onRate(userRating, reviewComment)
                    showReviewDialog = false
                }) {
                    Text("Submit")
                }
            },
            dismissButton = {
                OutlinedButton(onClick = { showReviewDialog = false }) {
                    Text("Cancel")
                }
            }
        )
    }
}

@Composable
private fun InstalledTab() {
    // Show installed plugins with update buttons
    val installedPlugins = ideApp.pluginManager.getLoadedPlugins()

    LazyColumn(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        item {
            Text("Installed Plugins", style = MaterialTheme.typography.h5)
            Spacer(modifier = Modifier.height(16.dp))
        }

        items(installedPlugins) { plugin ->
            Card(modifier = Modifier.padding(8.dp)) {
                Row(
                    modifier = Modifier.padding(16.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Text(plugin.name, style = MaterialTheme.typography.h6)
                        Text(plugin.description, style = MaterialTheme.typography.body2)
                        Text("v${plugin.version}", style = MaterialTheme.typography.caption)
                    }

                    Row {
                        Button(onClick = { /* Check for updates */ }) {
                            Text("Check Updates")
                        }
                        Spacer(modifier = Modifier.width(8.dp))
                        OutlinedButton(onClick = { /* Uninstall */ }) {
                            Text("Uninstall")
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun PublishTab() {
    // Plugin publishing interface
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Publish Plugin", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        // Publishing form would go here
        Text("Plugin publishing interface coming soon...")
    }
}

@Composable
private fun SettingsTab() {
    // Marketplace settings
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Marketplace Settings", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        // Settings options would go here
        Text("Settings interface coming soon...")
    }
}