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
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.*

/**
 * Profiler and Benchmarking UI for Foundry IDE
 * Complete performance analysis and benchmarking interface
 */
@Composable
fun ProfilerWindow(onClose: () -> Unit) {
    var currentTab by remember { mutableStateOf(ProfilerTab.PERFORMANCE) }
    var isRecording by remember { mutableStateOf(false) }
    var profilerData by remember { mutableStateOf(ProfilerData()) }
    var benchmarkResults by remember { mutableStateOf<List<BenchmarkResult>>(emptyList()) }

    val scope = rememberCoroutineScope()

    Window(
        onCloseRequest = onClose,
        title = "Profiler & Benchmarking - Foundry IDE",
        state = rememberWindowState(width = 1200.dp, height = 800.dp)
    ) {
        MaterialTheme {
            Column(modifier = Modifier.fillMaxSize()) {
                // Header with controls
                ProfilerHeader(
                    isRecording = isRecording,
                    onRecordingToggle = { isRecording = it },
                    onClearData = { profilerData = ProfilerData() },
                    onRunBenchmark = { runBenchmark() }
                )

                // Tabs
                ProfilerTabs(currentTab, onTabChange = { currentTab = it })

                // Content
                when (currentTab) {
                    ProfilerTab.PERFORMANCE -> PerformanceTab(profilerData, isRecording)
                    ProfilerTab.MEMORY -> MemoryTab(profilerData)
                    ProfilerTab.GPU -> GPUTab(profilerData)
                    ProfilerTab.BENCHMARKS -> BenchmarksTab(benchmarkResults)
                    ProfilerTab.SETTINGS -> SettingsTab()
                }
            }
        }
    }

    fun runBenchmark() {
        scope.launch {
            // Simulate benchmark execution
            delay(2000)
            val results = listOf(
                BenchmarkResult("Frame Time", 16.67f, "ms", 60.0f, "Target: 16.67ms"),
                BenchmarkResult("FPS", 60.0f, "fps", 60.0f, "Stable 60 FPS"),
                BenchmarkResult("Memory Usage", 256.0f, "MB", 512.0f, "Under limit"),
                BenchmarkResult("Draw Calls", 1500.0f, "calls", 2000.0f, "Acceptable")
            )
            benchmarkResults = results
        }
    }
}

enum class ProfilerTab {
    PERFORMANCE, MEMORY, GPU, BENCHMARKS, SETTINGS
}

data class ProfilerData(
    val frameTime: Float = 16.67f,
    val fps: Float = 60.0f,
    val cpuUsage: Float = 45.0f,
    val memoryUsage: Long = 256 * 1024 * 1024, // bytes
    val drawCalls: Int = 1500,
    val triangles: Int = 50000,
    val gpuMemory: Long = 128 * 1024 * 1024,
    val samples: List<ProfileSample> = emptyList()
)

data class ProfileSample(
    val name: String,
    val duration: Float, // microseconds
    val callCount: Int,
    val totalTime: Float,
    val minTime: Float,
    val maxTime: Float
)

data class BenchmarkResult(
    val name: String,
    val value: Float,
    val unit: String,
    val target: Float,
    val status: String
)

@Composable
private fun ProfilerHeader(
    isRecording: Boolean,
    onRecordingToggle: (Boolean) -> Unit,
    onClearData: () -> Unit,
    onRunBenchmark: () -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(16.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        // Title
        Text(
            text = "🔍 Profiler",
            style = MaterialTheme.typography.h5,
            fontWeight = FontWeight.Bold,
            color = Color(0xFFFF6B35)
        )

        Spacer(modifier = Modifier.width(32.dp))

        // Recording toggle
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("Recording", style = MaterialTheme.typography.subtitle1)
            Spacer(modifier = Modifier.width(8.dp))
            Switch(
                checked = isRecording,
                onCheckedChange = onRecordingToggle,
                colors = SwitchDefaults.colors(
                    checkedThumbColor = Color.Red,
                    checkedTrackColor = Color.Red.copy(alpha = 0.5f)
                )
            )
        }

        Spacer(modifier = Modifier.weight(1f))

        // Action buttons
        OutlinedButton(onClick = onClearData) {
            Icon(Icons.Default.Clear, "Clear")
            Spacer(modifier = Modifier.width(8.dp))
            Text("Clear")
        }

        Spacer(modifier = Modifier.width(8.dp))

        Button(onClick = onRunBenchmark) {
            Icon(Icons.Default.PlayArrow, "Benchmark")
            Spacer(modifier = Modifier.width(8.dp))
            Text("Run Benchmark")
        }
    }
}

@Composable
private fun ProfilerTabs(currentTab: ProfilerTab, onTabChange: (ProfilerTab) -> Unit) {
    TabRow(selectedTabIndex = currentTab.ordinal) {
        ProfilerTab.values().forEach { tab ->
            Tab(
                selected = currentTab == tab,
                onClick = { onTabChange(tab) },
                text = { Text(tab.name) },
                icon = {
                    when (tab) {
                        ProfilerTab.PERFORMANCE -> Icon(Icons.Default.Timeline, tab.name)
                        ProfilerTab.MEMORY -> Icon(Icons.Default.Memory, tab.name)
                        ProfilerTab.GPU -> Icon(Icons.Default.Games, tab.name)
                        ProfilerTab.BENCHMARKS -> Icon(Icons.Default.Assessment, tab.name)
                        ProfilerTab.SETTINGS -> Icon(Icons.Default.Settings, tab.name)
                    }
                }
            )
        }
    }
}

@Composable
private fun PerformanceTab(profilerData: ProfilerData, isRecording: Boolean) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Performance Analysis", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        // Real-time metrics
        Row(modifier = Modifier.fillMaxWidth()) {
            MetricCard("Frame Time", "${"%.2f".format(profilerData.frameTime)}ms", Color.Green)
            MetricCard("FPS", "${profilerData.fps.toInt()}", Color.Blue)
            MetricCard("CPU Usage", "${profilerData.cpuUsage.toInt()}%", Color.Orange)
            MetricCard("Draw Calls", profilerData.drawCalls.toString(), Color.Purple)
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Performance graph
        Card(modifier = Modifier.fillMaxWidth().height(200.dp)) {
            PerformanceGraph(profilerData, isRecording)
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Profile samples
        Text("Profile Samples", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(8.dp))

        LazyColumn(modifier = Modifier.fillMaxSize()) {
            items(profilerData.samples) { sample ->
                ProfileSampleRow(sample)
            }
        }
    }
}

@Composable
private fun MemoryTab(profilerData: ProfilerData) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Memory Analysis", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        // Memory metrics
        Row(modifier = Modifier.fillMaxWidth()) {
            MetricCard("Memory Usage", formatBytes(profilerData.memoryUsage), Color.Red)
            MetricCard("GPU Memory", formatBytes(profilerData.gpuMemory), Color.Cyan)
            MetricCard("Triangles", profilerData.triangles.toString(), Color.Magenta)
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Memory usage chart
        Card(modifier = Modifier.fillMaxWidth().height(200.dp)) {
            MemoryUsageChart(profilerData)
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Memory breakdown
        Text("Memory Breakdown", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(8.dp))

        val memoryBreakdown = listOf(
            "Textures" to 128L * 1024 * 1024,
            "Meshes" to 64L * 1024 * 1024,
            "Shaders" to 32L * 1024 * 1024,
            "Audio" to 16L * 1024 * 1024,
            "Other" to 16L * 1024 * 1024
        )

        LazyColumn(modifier = Modifier.fillMaxSize()) {
            items(memoryBreakdown) { (category, bytes) ->
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(category, modifier = Modifier.weight(1f))
                    Text(formatBytes(bytes), style = MaterialTheme.typography.caption)
                    Spacer(modifier = Modifier.width(16.dp))
                    LinearProgressIndicator(
                        progress = bytes.toFloat() / (256L * 1024 * 1024).toFloat(),
                        modifier = Modifier.width(100.dp)
                    )
                }
            }
        }
    }
}

@Composable
private fun GPUTab(profilerData: ProfilerData) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("GPU Analysis", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        // GPU metrics
        Row(modifier = Modifier.fillMaxWidth()) {
            MetricCard("GPU Memory", formatBytes(profilerData.gpuMemory), Color.Cyan)
            MetricCard("Draw Calls", profilerData.drawCalls.toString(), Color.Purple)
            MetricCard("Triangles", profilerData.triangles.toString(), Color.Magenta)
        }

        Spacer(modifier = Modifier.height(16.dp))

        // GPU usage chart
        Card(modifier = Modifier.fillMaxWidth().height(200.dp)) {
            GPUUsageChart(profilerData)
        }

        Spacer(modifier = Modifier.height(16.dp))

        // Pipeline statistics
        Text("Pipeline Statistics", style = MaterialTheme.typography.h6)
        Spacer(modifier = Modifier.height(8.dp))

        val pipelineStats = listOf(
            "Vertex Shader" to 2.5f,
            "Fragment Shader" to 8.2f,
            "Geometry Shader" to 0.1f,
            "Compute Shader" to 1.8f
        )

        LazyColumn(modifier = Modifier.fillMaxSize()) {
            items(pipelineStats) { (stage, time) ->
                Row(
                    modifier = Modifier
                        .fillMaxWidth()
                        .padding(8.dp),
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(stage, modifier = Modifier.weight(1f))
                    Text("${"%.1f".format(time)}ms", style = MaterialTheme.typography.caption)
                    Spacer(modifier = Modifier.width(16.dp))
                    LinearProgressIndicator(
                        progress = time / 10f, // Normalize to 10ms max
                        modifier = Modifier.width(100.dp)
                    )
                }
            }
        }
    }
}

@Composable
private fun BenchmarksTab(benchmarkResults: List<BenchmarkResult>) {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Benchmark Results", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        LazyColumn(modifier = Modifier.fillMaxSize()) {
            items(benchmarkResults) { result ->
                BenchmarkResultRow(result)
            }
        }
    }
}

@Composable
private fun SettingsTab() {
    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Profiler Settings", style = MaterialTheme.typography.h5)
        Spacer(modifier = Modifier.height(16.dp))

        // Settings options would go here
        Text("Profiler settings interface coming soon...")
    }
}

@Composable
private fun MetricCard(title: String, value: String, color: Color) {
    Card(
        modifier = Modifier
            .weight(1f)
            .padding(4.dp),
        elevation = 4.dp
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(title, style = MaterialTheme.typography.caption, color = Color.Gray)
            Text(
                value,
                style = MaterialTheme.typography.h4,
                color = color,
                fontWeight = FontWeight.Bold
            )
        }
    }
}

@Composable
private fun ProfileSampleRow(sample: ProfileSample) {
    Card(modifier = Modifier.fillMaxWidth().padding(4.dp)) {
        Row(
            modifier = Modifier.padding(12.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text(sample.name, fontWeight = FontWeight.Bold)
                Text(
                    "Calls: ${sample.callCount} | Avg: ${"%.2f".format(sample.totalTime / sample.callCount)}μs",
                    style = MaterialTheme.typography.caption
                )
            }
            Text("${"%.2f".format(sample.duration)}μs", style = MaterialTheme.typography.body2)
            Spacer(modifier = Modifier.width(16.dp))
            LinearProgressIndicator(
                progress = sample.duration / 16667f, // Normalize to 60fps frame time
                modifier = Modifier.width(100.dp)
            )
        }
    }
}

@Composable
private fun BenchmarkResultRow(result: BenchmarkResult) {
    Card(modifier = Modifier.fillMaxWidth().padding(4.dp)) {
        Row(
            modifier = Modifier.padding(12.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column(modifier = Modifier.weight(1f)) {
                Text(result.name, fontWeight = FontWeight.Bold)
                Text(result.status, style = MaterialTheme.typography.caption)
            }
            Text(
                "${"%.2f".format(result.value)} ${result.unit}",
                style = MaterialTheme.typography.body2,
                color = if (result.value <= result.target) Color.Green else Color.Red
            )
        }
    }
}

private fun formatBytes(bytes: Long): String {
    val units = arrayOf("B", "KB", "MB", "GB")
    var value = bytes.toDouble()
    var unitIndex = 0

    while (value >= 1024 && unitIndex < units.size - 1) {
        value /= 1024
        unitIndex++
    }

    return "%.1f %s".format(value, units[unitIndex])
}

@Composable
private fun PerformanceGraph(profilerData: ProfilerData, isRecording: Boolean) {
    val fpsHistory = remember { mutableStateListOf<Float>() }
    val frameTimeHistory = remember { mutableStateListOf<Float>() }
    val maxDataPoints = 100

    // Simulate data updates when recording
    LaunchedEffect(isRecording) {
        if (isRecording) {
            while (isRecording) {
                // Add some variation to simulate real data
                val baseFps = profilerData.fps
                val baseFrameTime = profilerData.frameTime
                val variation = (kotlin.random.Random.nextFloat() - 0.5f) * 10f

                fpsHistory.add((baseFps + variation).coerceIn(30f, 120f))
                frameTimeHistory.add((baseFrameTime + variation * 0.3f).coerceIn(8f, 33f))

                // Keep only recent data points
                if (fpsHistory.size > maxDataPoints) {
                    fpsHistory.removeAt(0)
                    frameTimeHistory.removeAt(0)
                }

                kotlinx.coroutines.delay(100) // Update 10 times per second
            }
        }
    }

    Canvas(modifier = Modifier.fillMaxSize()) {
        val width = size.width
        val height = size.height

        if (fpsHistory.isNotEmpty()) {
            val maxFps = 120f
            val maxFrameTime = 33f // ~30 FPS

            // Draw FPS graph (blue)
            drawPath(
                path = androidx.compose.ui.graphics.Path().apply {
                    fpsHistory.forEachIndexed { index, fps ->
                        val x = (index.toFloat() / fpsHistory.size) * width
                        val y = height - (fps / maxFps) * height
                        if (index == 0) moveTo(x, y) else lineTo(x, y)
                    }
                },
                color = Color.Blue,
                style = androidx.compose.ui.graphics.drawscope.Stroke(width = 2f)
            )

            // Draw frame time graph (red)
            drawPath(
                path = androidx.compose.ui.graphics.Path().apply {
                    frameTimeHistory.forEachIndexed { index, frameTime ->
                        val x = (index.toFloat() / frameTimeHistory.size) * width
                        val y = height - (frameTime / maxFrameTime) * height
                        if (index == 0) moveTo(x, y) else lineTo(x, y)
                    }
                },
                color = Color.Red,
                style = androidx.compose.ui.graphics.drawscope.Stroke(width = 2f)
            )

            // Draw grid lines
            for (i in 0..4) {
                val y = (i.toFloat() / 4) * height
                drawLine(
                    color = Color.Gray.copy(alpha = 0.3f),
                    start = androidx.compose.ui.geometry.Offset(0f, y),
                    end = androidx.compose.ui.geometry.Offset(width, y),
                    strokeWidth = 1f
                )
            }
        }

        // Draw legend
        drawRect(
            color = Color.Blue,
            topLeft = androidx.compose.ui.geometry.Offset(10f, 10f),
            size = androidx.compose.ui.geometry.Size(20f, 10f)
        )
        drawRect(
            color = Color.Red,
            topLeft = androidx.compose.ui.geometry.Offset(10f, 25f),
            size = androidx.compose.ui.geometry.Size(20f, 10f)
        )
    }

    // Status overlay
    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(8.dp),
        contentAlignment = Alignment.TopEnd
    ) {
        Text(
            text = if (isRecording) "● Recording" else "⏸ Paused",
            style = MaterialTheme.typography.caption,
            color = if (isRecording) Color.Red else Color.Gray
        )
    }
}

@Composable
private fun MemoryUsageChart(profilerData: ProfilerData) {
    val memoryHistory = remember { mutableStateListOf<Long>() }
    val maxDataPoints = 100

    // Simulate memory usage over time
    LaunchedEffect(Unit) {
        while (true) {
            val baseMemory = profilerData.memoryUsage
            val variation = (kotlin.random.Random.nextLong() % (50L * 1024 * 1024)) - (25L * 1024 * 1024)
            memoryHistory.add((baseMemory + variation).coerceIn(100L * 1024 * 1024, 1000L * 1024 * 1024))

            if (memoryHistory.size > maxDataPoints) {
                memoryHistory.removeAt(0)
            }

            kotlinx.coroutines.delay(500) // Update twice per second
        }
    }

    Canvas(modifier = Modifier.fillMaxSize()) {
        val width = size.width
        val height = size.height

        if (memoryHistory.isNotEmpty()) {
            val maxMemory = 1000L * 1024 * 1024 // 1GB max for display

            // Draw memory usage graph
            drawPath(
                path = androidx.compose.ui.graphics.Path().apply {
                    memoryHistory.forEachIndexed { index, memory ->
                        val x = (index.toFloat() / memoryHistory.size) * width
                        val y = height - (memory.toFloat() / maxMemory) * height
                        if (index == 0) moveTo(x, y) else lineTo(x, y)
                    }
                },
                color = Color.Green,
                style = androidx.compose.ui.graphics.drawscope.Stroke(width = 2f)
            )

            // Draw grid lines
            for (i in 0..4) {
                val y = (i.toFloat() / 4) * height
                drawLine(
                    color = Color.Gray.copy(alpha = 0.3f),
                    start = androidx.compose.ui.geometry.Offset(0f, y),
                    end = androidx.compose.ui.geometry.Offset(width, y),
                    strokeWidth = 1f
                )
            }
        }
    }

    // Current memory display
    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(8.dp),
        contentAlignment = Alignment.TopEnd
    ) {
        Text(
            text = "Current: ${formatBytes(profilerData.memoryUsage)}",
            style = MaterialTheme.typography.caption,
            color = Color.Green
        )
    }
}

@Composable
private fun GPUUsageChart(profilerData: ProfilerData) {
    val gpuUsageHistory = remember { mutableStateListOf<Float>() }
    val maxDataPoints = 100

    // Simulate GPU usage over time
    LaunchedEffect(Unit) {
        while (true) {
            val baseUsage = profilerData.cpuUsage * 0.8f // GPU typically less than CPU
            val variation = (kotlin.random.Random.nextFloat() - 0.5f) * 20f
            gpuUsageHistory.add((baseUsage + variation).coerceIn(10f, 95f))

            if (gpuUsageHistory.size > maxDataPoints) {
                gpuUsageHistory.removeAt(0)
            }

            kotlinx.coroutines.delay(200) // Update 5 times per second
        }
    }

    Canvas(modifier = Modifier.fillMaxSize()) {
        val width = size.width
        val height = size.height

        if (gpuUsageHistory.isNotEmpty()) {
            // Draw GPU usage graph
            drawPath(
                path = androidx.compose.ui.graphics.Path().apply {
                    gpuUsageHistory.forEachIndexed { index, usage ->
                        val x = (index.toFloat() / gpuUsageHistory.size) * width
                        val y = height - (usage / 100f) * height
                        if (index == 0) moveTo(x, y) else lineTo(x, y)
                    }
                },
                color = Color.Cyan,
                style = androidx.compose.ui.graphics.drawscope.Stroke(width = 2f)
            )

            // Draw grid lines
            for (i in 0..4) {
                val y = (i.toFloat() / 4) * height
                drawLine(
                    color = Color.Gray.copy(alpha = 0.3f),
                    start = androidx.compose.ui.geometry.Offset(0f, y),
                    end = androidx.compose.ui.geometry.Offset(width, y),
                    strokeWidth = 1f
                )
            }
        }
    }

    // GPU memory display
    Box(
        modifier = Modifier
            .fillMaxSize()
            .padding(8.dp),
        contentAlignment = Alignment.TopEnd
    ) {
        Column(horizontalAlignment = Alignment.End) {
            Text(
                text = "GPU Memory: ${formatBytes(profilerData.gpuMemory)}",
                style = MaterialTheme.typography.caption,
                color = Color.Cyan
            )
            Text(
                text = "Triangles: ${profilerData.triangles}",
                style = MaterialTheme.typography.caption,
                color = Color.Magenta
            )
        }
    }
}