package com.foundry.ide.managers

import com.foundry.ide.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.time.LocalDateTime
import java.time.format.DateTimeFormatter

/**
 * Debug management system for Foundry IDE
 * Handles debugging, profiling, and performance monitoring
 */
@Serializable
data class DebugSession(
    val id: String,
    val name: String,
    val target: String,
    val startTime: Long,
    val endTime: Long? = null,
    val status: DebugStatus,
    val breakpoints: List<Breakpoint> = emptyList(),
    val callStack: List<CallFrame> = emptyList(),
    val variables: Map<String, VariableInfo> = emptyMap(),
    val output: List<String> = emptyList()
)

enum class DebugStatus {
    STARTING, RUNNING, PAUSED, STOPPED, ERROR
}

@Serializable
data class Breakpoint(
    val id: String,
    val file: String,
    val line: Int,
    val condition: String? = null,
    val enabled: Boolean = true
)

@Serializable
data class CallFrame(
    val function: String,
    val file: String,
    val line: Int,
    val column: Int
)

@Serializable
data class VariableInfo(
    val name: String,
    val value: String,
    val type: String,
    val scope: String
)

@Serializable
data class DebugEvent(
    val type: DebugEventType,
    val timestamp: Long,
    val message: String,
    val data: Map<String, String> = emptyMap()
)

enum class DebugEventType {
    BREAKPOINT_HIT, STEP_COMPLETED, EXCEPTION, OUTPUT, ERROR
}

class DebugManager {
    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
    }

    private val activeSessions = mutableMapOf<String, DebugSession>()
    private val breakpoints = mutableMapOf<String, Breakpoint>()
    private val debugHistory = mutableListOf<DebugSession>()
    private val eventListeners = mutableListOf<(DebugEvent) -> Unit>()

    private val logsDir = Paths.get("ide/logs")
    private val profilingDir = Paths.get("ide/profiling")

    init {
        ensureDirectoriesExist()
    }

    /**
     * Start debug session
     */
    fun startDebugSession(
        projectInfo: ProjectInfo,
        target: String,
        name: String? = null
    ): String {
        val sessionId = generateSessionId()
        val sessionName = name ?: "Debug Session ${LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss"))}"

        val debugSession = DebugSession(
            id = sessionId,
            name = sessionName,
            target = target,
            startTime = System.currentTimeMillis(),
            status = DebugStatus.STARTING
        )

        activeSessions[sessionId] = debugSession

        // Notify listeners
        notifyEvent(DebugEvent(
            type = DebugEventType.OUTPUT,
            timestamp = System.currentTimeMillis(),
            message = "Debug session started: $sessionName"
        ))

        return sessionId
    }

    /**
     * Stop debug session
     */
    fun stopDebugSession(sessionId: String): Boolean {
        return try {
            val session = activeSessions[sessionId] ?: return false

            val endedSession = session.copy(
                endTime = System.currentTimeMillis(),
                status = DebugStatus.STOPPED
            )

            activeSessions.remove(sessionId)
            debugHistory.add(endedSession)

            // Keep only last 50 sessions in history
            if (debugHistory.size > 50) {
                debugHistory.removeAt(0)
            }

            // Save debug log
            saveDebugLog(endedSession)

            // Notify listeners
            notifyEvent(DebugEvent(
                type = DebugEventType.OUTPUT,
                timestamp = System.currentTimeMillis(),
                message = "Debug session ended: ${session.name}"
            ))

            true
        } catch (e: Exception) {
            println("Failed to stop debug session: ${e.message}")
            false
        }
    }

    /**
     * Add breakpoint
     */
    fun addBreakpoint(file: String, line: Int, condition: String? = null): String {
        val breakpointId = generateBreakpointId()
        val breakpoint = Breakpoint(
            id = breakpointId,
            file = file,
            line = line,
            condition = condition,
            enabled = true
        )

        breakpoints[breakpointId] = breakpoint

        // Notify listeners
        notifyEvent(DebugEvent(
            type = DebugEventType.OUTPUT,
            timestamp = System.currentTimeMillis(),
            message = "Breakpoint added: $file:$line"
        ))

        return breakpointId
    }

    /**
     * Remove breakpoint
     */
    fun removeBreakpoint(breakpointId: String): Boolean {
        val breakpoint = breakpoints.remove(breakpointId) ?: return false

        // Notify listeners
        notifyEvent(DebugEvent(
            type = DebugEventType.OUTPUT,
            timestamp = System.currentTimeMillis(),
            message = "Breakpoint removed: ${breakpoint.file}:${breakpoint.line}"
        ))

        return true
    }

    /**
     * Enable/disable breakpoint
     */
    fun toggleBreakpoint(breakpointId: String): Boolean {
        val breakpoint = breakpoints[breakpointId] ?: return false

        val updatedBreakpoint = breakpoint.copy(enabled = !breakpoint.enabled)
        breakpoints[breakpointId] = updatedBreakpoint

        // Notify listeners
        notifyEvent(DebugEvent(
            type = DebugEventType.OUTPUT,
            timestamp = System.currentTimeMillis(),
            message = "Breakpoint ${if (updatedBreakpoint.enabled) "enabled" else "disabled"}: ${breakpoint.file}:${breakpoint.line}"
        ))

        return true
    }

    /**
     * Step through code
     */
    fun step(sessionId: String, stepType: StepType): Boolean {
        val session = activeSessions[sessionId] ?: return false

        return try {
            when (stepType) {
                StepType.INTO -> performStepInto(session)
                StepType.OVER -> performStepOver(session)
                StepType.OUT -> performStepOut(session)
            }

            // Notify listeners
            notifyEvent(DebugEvent(
                type = DebugEventType.STEP_COMPLETED,
                timestamp = System.currentTimeMillis(),
                message = "Step completed: $stepType"
            ))

            true
        } catch (e: Exception) {
            notifyEvent(DebugEvent(
                type = DebugEventType.ERROR,
                timestamp = System.currentTimeMillis(),
                message = "Step failed: ${e.message}"
            ))
            false
        }
    }

    /**
     * Continue execution
     */
    fun continueExecution(sessionId: String): Boolean {
        val session = activeSessions[sessionId] ?: return false

        return try {
            // In a real implementation, this would resume the debugger

            // Notify listeners
            notifyEvent(DebugEvent(
                type = DebugEventType.OUTPUT,
                timestamp = System.currentTimeMillis(),
                message = "Execution continued"
            ))

            true
        } catch (e: Exception) {
            notifyEvent(DebugEvent(
                type = DebugEventType.ERROR,
                timestamp = System.currentTimeMillis(),
                message = "Continue failed: ${e.message}"
            ))
            false
        }
    }

    /**
     * Pause execution
     */
    fun pauseExecution(sessionId: String): Boolean {
        val session = activeSessions[sessionId] ?: return false

        return try {
            // In a real implementation, this would pause the debugger

            // Notify listeners
            notifyEvent(DebugEvent(
                type = DebugEventType.OUTPUT,
                timestamp = System.currentTimeMillis(),
                message = "Execution paused"
            ))

            true
        } catch (e: Exception) {
            notifyEvent(DebugEvent(
                type = DebugEventType.ERROR,
                timestamp = System.currentTimeMillis(),
                message = "Pause failed: ${e.message}"
            ))
            false
        }
    }

    /**
     * Get current call stack
     */
    fun getCallStack(sessionId: String): List<CallFrame> {
        val session = activeSessions[sessionId] ?: return emptyList()

        // In a real implementation, this would get the actual call stack from the debugger
        return session.callStack.ifEmpty {
            listOf(
                CallFrame(
                    function = "main",
                    file = "main.kt",
                    line = 1,
                    column = 1
                )
            )
        }
    }

    /**
     * Get current variables
     */
    fun getVariables(sessionId: String): Map<String, VariableInfo> {
        val session = activeSessions[sessionId] ?: return emptyMap()

        // In a real implementation, this would get the actual variables from the debugger
        return session.variables.ifEmpty {
            mapOf(
                "gameTime" to VariableInfo(
                    name = "gameTime",
                    value = "0.0",
                    type = "Float",
                    scope = "local"
                ),
                "player" to VariableInfo(
                    name = "player",
                    value = "Player@12345",
                    type = "Player",
                    scope = "local"
                )
            )
        }
    }

    /**
     * Evaluate expression
     */
    fun evaluateExpression(sessionId: String, expression: String): String {
        val session = activeSessions[sessionId] ?: return "Session not found"

        return try {
            // In a real implementation, this would evaluate the expression in the debugger context
            // For now, return a mock result
            "Expression result: $expression = ${kotlin.random.Random.nextInt(100)}"
        } catch (e: Exception) {
            "Evaluation failed: ${e.message}"
        }
    }

    /**
     * Start profiling session
     */
    fun startProfiling(target: String): String {
        val sessionId = "profile_${generateSessionId()}"

        // Notify listeners
        notifyEvent(DebugEvent(
            type = DebugEventType.OUTPUT,
            timestamp = System.currentTimeMillis(),
            message = "Profiling started for target: $target"
        ))

        return sessionId
    }

    /**
     * Stop profiling session
     */
    fun stopProfiling(sessionId: String): ProfilingResult? {
        return try {
            // In a real implementation, this would collect profiling data

            val result = ProfilingResult(
                sessionId = sessionId,
                duration = 5000L,
                memoryUsage = MemoryInfo(
                    heapUsed = 256 * 1024 * 1024,
                    heapTotal = 512 * 1024 * 1024,
                    nativeUsed = 128 * 1024 * 1024
                ),
                performanceMetrics = mapOf(
                    "fps" to "60.0",
                    "frameTime" to "16.67ms",
                    "drawCalls" to "150",
                    "triangles" to "50000"
                ),
                hotspots = listOf(
                    Hotspot(
                        function = "PhysicsSystem::update",
                        file = "PhysicsSystem.cpp",
                        line = 45,
                        percentage = 35.5
                    ),
                    Hotspot(
                        function = "RenderingSystem::render",
                        file = "RenderingSystem.cpp",
                        line = 120,
                        percentage = 28.3
                    )
                )
            )

            // Notify listeners
            notifyEvent(DebugEvent(
                type = DebugEventType.OUTPUT,
                timestamp = System.currentTimeMillis(),
                message = "Profiling completed"
            ))

            result
        } catch (e: Exception) {
            println("Failed to stop profiling: ${e.message}")
            null
        }
    }

    /**
     * Get debug sessions
     */
    fun getActiveSessions(): List<DebugSession> {
        return activeSessions.values.toList()
    }

    /**
     * Get debug history
     */
    fun getDebugHistory(limit: Int = 20): List<DebugSession> {
        return debugHistory.takeLast(limit).reversed()
    }

    /**
     * Get breakpoints
     */
    fun getBreakpoints(): List<Breakpoint> {
        return breakpoints.values.toList()
    }

    /**
     * Add debug event listener
     */
    fun addEventListener(listener: (DebugEvent) -> Unit) {
        eventListeners.add(listener)
    }

    /**
     * Remove debug event listener
     */
    fun removeEventListener(listener: (DebugEvent) -> Unit) {
        eventListeners.remove(listener)
    }

    /**
     * Clear debug history
     */
    fun clearHistory(): Boolean {
        return try {
            debugHistory.clear()
            true
        } catch (e: Exception) {
            println("Failed to clear debug history: ${e.message}")
            false
        }
    }

    /**
     * Export debug log
     */
    fun exportDebugLog(sessionId: String, filePath: String): Boolean {
        return try {
            val session = debugHistory.find { it.id == sessionId }
                ?: activeSessions[sessionId]
                ?: return false

            val logContent = buildDebugLogContent(session)
            File(filePath).writeText(logContent)
            true
        } catch (e: Exception) {
            println("Failed to export debug log: ${e.message}")
            false
        }
    }

    /**
     * Perform step into
     */
    private fun performStepInto(session: DebugSession): Boolean {
        // In a real implementation, this would step into the current function
        println("Stepping into...")
        return true
    }

    /**
     * Perform step over
     */
    private fun performStepOver(session: DebugSession): Boolean {
        // In a real implementation, this would step over the current line
        println("Stepping over...")
        return true
    }

    /**
     * Perform step out
     */
    private fun performStepOut(session: DebugSession): Boolean {
        // In a real implementation, this would step out of the current function
        println("Stepping out...")
        return true
    }

    /**
     * Notify event listeners
     */
    private fun notifyEvent(event: DebugEvent) {
        eventListeners.forEach { listener ->
            try {
                listener(event)
            } catch (e: Exception) {
                println("Debug event listener error: ${e.message}")
            }
        }
    }

    /**
     * Save debug log
     */
    private fun saveDebugLog(session: DebugSession) {
        try {
            val logContent = buildDebugLogContent(session)
            val logFile = logsDir.resolve("debug_${session.id}.log")
            Files.writeString(logFile, logContent)
        } catch (e: Exception) {
            println("Failed to save debug log: ${e.message}")
        }
    }

    /**
     * Build debug log content
     */
    private fun buildDebugLogContent(session: DebugSession): String {
        val content = StringBuilder()
        content.append("Foundry IDE Debug Log\n")
        content.append("=====================\n\n")
        content.append("Session: ${session.name}\n")
        content.append("ID: ${session.id}\n")
        content.append("Target: ${session.target}\n")
        content.append("Start Time: ${LocalDateTime.ofEpochSecond(session.startTime / 1000, 0, java.time.ZoneOffset.UTC)}\n")

        if (session.endTime != null) {
            content.append("End Time: ${LocalDateTime.ofEpochSecond(session.endTime / 1000, 0, java.time.ZoneOffset.UTC)}\n")
            content.append("Duration: ${session.endTime - session.startTime}ms\n")
        }

        content.append("Status: ${session.status}\n")
        content.append("Breakpoints: ${session.breakpoints.size}\n\n")

        if (session.output.isNotEmpty()) {
            content.append("Output:\n")
            content.append("------\n")
            session.output.forEach { line ->
                content.append("$line\n")
            }
            content.append("\n")
        }

        return content.toString()
    }

    /**
     * Generate unique session ID
     */
    private fun generateSessionId(): String {
        val timestamp = System.currentTimeMillis()
        return "debug_${timestamp}_${kotlin.random.Random.nextInt(1000)}"
    }

    /**
     * Generate unique breakpoint ID
     */
    private fun generateBreakpointId(): String {
        val timestamp = System.currentTimeMillis()
        return "bp_${timestamp}_${kotlin.random.Random.nextInt(1000)}"
    }

    /**
     * Ensure required directories exist
     */
    private fun ensureDirectoriesExist() {
        Files.createDirectories(logsDir)
        Files.createDirectories(profilingDir)
    }
}

/**
 * Step types for debugging
 */
enum class StepType {
    INTO, OVER, OUT
}

/**
 * Profiling result data class
 */
@Serializable
data class ProfilingResult(
    val sessionId: String,
    val duration: Long,
    val memoryUsage: MemoryInfo,
    val performanceMetrics: Map<String, String>,
    val hotspots: List<Hotspot>
)

@Serializable
data class MemoryInfo(
    val heapUsed: Long,
    val heapTotal: Long,
    val nativeUsed: Long
)

@Serializable
data class Hotspot(
    val function: String,
    val file: String,
    val line: Int,
    val percentage: Double
)
