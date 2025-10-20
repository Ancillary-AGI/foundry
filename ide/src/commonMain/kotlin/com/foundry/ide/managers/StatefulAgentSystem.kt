package com.foundry.ide.managers

import com.foundry.ide.*
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.*
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json
import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.util.concurrent.ConcurrentHashMap
import kotlin.random.Random

/**
 * Advanced Stateful Multi-Agent System for Foundry IDE
 * Supports persistent memory, agent collaboration, and shared knowledge
 */
class StatefulAgentSystem(
    private val workspacePath: String = System.getProperty("user.dir") ?: "./"
) {
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private val json = Json {
        prettyPrint = true
        ignoreUnknownKeys = true
        encodeDefaults = true
    }

    // Agent management
    private val activeAgents = ConcurrentHashMap<String, StatefulAgent>()
    private val agentStates = ConcurrentHashMap<String, AgentState>()

    // Shared memory system
    private val sharedMemory = SharedMemorySystem()
    private val agentCommunication = AgentCommunicationSystem()

    // Persistence
    private val agentStatePath = Paths.get(workspacePath, ".foundry", "agents", "states")
    private val sharedMemoryPath = Paths.get(workspacePath, ".foundry", "agents", "shared_memory")

    init {
        ensureDirectoriesExist()
        loadPersistedStates()
        initializeDefaultAgents()
    }

    /**
     * Initialize default agents
     */
    private fun initializeDefaultAgents() {
        // Code Generation Agent
        createAgent("code_generator", AgentRole.CODE_GENERATION, listOf(
            "kotlin", "java", "cpp", "python", "javascript"
        ))

        // Code Analysis Agent
        createAgent("code_analyzer", AgentRole.CODE_ANALYSIS, listOf(
            "static_analysis", "performance_analysis", "security_analysis"
        ))

        // Architecture Agent
        createAgent("architect", AgentRole.ARCHITECTURE, listOf(
            "design_patterns", "system_architecture", "scalability"
        ))

        // Testing Agent
        createAgent("tester", AgentRole.TESTING, listOf(
            "unit_tests", "integration_tests", "performance_tests"
        ))

        // Documentation Agent
        createAgent("documenter", AgentRole.DOCUMENTATION, listOf(
            "api_docs", "code_comments", "readme_generation"
        ))

        // Debugging Agent
        createAgent("debugger", AgentRole.DEBUGGING, listOf(
            "error_analysis", "bug_fixing", "performance_debugging"
        ))

        // Learning Agent (meta-agent that improves other agents)
        createAgent("learner", AgentRole.LEARNING, listOf(
            "pattern_recognition", "feedback_analysis", "optimization"
        ))
    }

    /**
     * Create a new stateful agent
     */
    fun createAgent(
        id: String,
        role: AgentRole,
        capabilities: List<String>,
        personality: AgentPersonality = AgentPersonality.BALANCED
    ): StatefulAgent {
        val agent = StatefulAgent(
            id = id,
            role = role,
            capabilities = capabilities,
            personality = personality,
            memory = AgentMemory(),
            learning = LearningSystem(),
            communication = AgentCommunication(id, agentCommunication)
        )

        activeAgents[id] = agent

        // Load persisted state if available
        val persistedState = agentStates[id]
        if (persistedState != null) {
            agent.loadState(persistedState)
        }

        // Register with communication system
        agentCommunication.registerAgent(agent)

        return agent
    }

    /**
     * Execute task with multi-agent collaboration
     */
    suspend fun executeCollaborativeTask(
        task: String,
        context: Map<String, Any> = emptyMap(),
        primaryAgentId: String? = null
    ): CollaborativeTaskResult {
        return withContext(Dispatchers.Default) {
            val startTime = System.currentTimeMillis()

            // Analyze task and determine required agents
            val taskAnalysis = analyzeTaskRequirements(task, context)
            val requiredAgents = selectAgentsForTask(taskAnalysis)

            // Create collaboration session
            val sessionId = "session_${System.currentTimeMillis()}_${Random.nextInt(1000)}"
            val session = CollaborationSession(
                id = sessionId,
                task = task,
                context = context,
                participants = requiredAgents.map { it.id },
                startTime = startTime
            )

            // Execute collaborative workflow
            val result = executeCollaborativeWorkflow(session, requiredAgents, taskAnalysis)

            // Update agent memories and learning
            updateAgentMemories(session, result, requiredAgents)

            // Persist states
            persistAgentStates(requiredAgents)

            result.copy(
                executionTime = System.currentTimeMillis() - startTime,
                collaborationMetrics = calculateCollaborationMetrics(session, result)
            )
        }
    }

    /**
     * Analyze task requirements
     */
    private fun analyzeTaskRequirements(task: String, context: Map<String, Any>): TaskAnalysis {
        val complexity = estimateComplexity(task)
        val domains = identifyRequiredDomains(task)
        val dependencies = identifyTaskDependencies(task, context)
        val estimatedDuration = estimateDuration(complexity, domains.size)

        return TaskAnalysis(
            task = task,
            complexity = complexity,
            requiredDomains = domains,
            dependencies = dependencies,
            estimatedDuration = estimatedDuration,
            riskLevel = assessRiskLevel(task, context)
        )
    }

    /**
     * Select appropriate agents for the task
     */
    private fun selectAgentsForTask(analysis: TaskAnalysis): List<StatefulAgent> {
        val candidates = activeAgents.values.filter { agent ->
            analysis.requiredDomains.any { domain ->
                agent.capabilities.contains(domain) || agent.role.domains.contains(domain)
            }
        }.toList()

        // Sort by relevance and past performance
        return candidates.sortedByDescending { agent ->
            calculateAgentRelevanceScore(agent, analysis)
        }.take(minOf(4, candidates.size)) // Limit to 4 agents max
    }

    /**
     * Execute collaborative workflow
     */
    private suspend fun executeCollaborativeWorkflow(
        session: CollaborationSession,
        agents: List<StatefulAgent>,
        analysis: TaskAnalysis
    ): CollaborativeTaskResult {
        val workflow = createWorkflowPlan(analysis, agents)
        val results = mutableMapOf<String, Any>()
        val communications = mutableListOf<AgentMessage>()

        // Execute workflow steps
        for (step in workflow.steps) {
            val stepResult = executeWorkflowStep(step, agents, session, communications)
            results[step.id] = stepResult

            // Allow agents to communicate and adapt
            processAgentCommunications(communications, agents, session)
        }

        // Synthesize final result
        val finalResult = synthesizeResults(results, analysis, agents)

        return CollaborativeTaskResult(
            sessionId = session.id,
            success = true,
            result = finalResult,
            participatingAgents = agents.map { it.id },
            workflowSteps = workflow.steps.size,
            communications = communications.size
        )
    }

    /**
     * Create workflow plan
     */
    private fun createWorkflowPlan(analysis: TaskAnalysis, agents: List<StatefulAgent>): WorkflowPlan {
        val steps = mutableListOf<WorkflowStep>()

        // Planning phase
        steps.add(WorkflowStep(
            id = "planning",
            description = "Analyze task and create detailed plan",
            assignedAgents = listOf(agents.first { it.role == AgentRole.ARCHITECT }.id),
            dependencies = emptyList(),
            estimatedDuration = 30
        ))

        // Execution phases based on domains
        analysis.requiredDomains.forEachIndexed { index, domain ->
            val relevantAgents = agents.filter { it.capabilities.contains(domain) }
            if (relevantAgents.isNotEmpty()) {
                steps.add(WorkflowStep(
                    id = "execute_$domain",
                    description = "Execute $domain specific tasks",
                    assignedAgents = relevantAgents.map { it.id },
                    dependencies = if (index > 0) listOf("execute_${analysis.requiredDomains[index - 1]}") else listOf("planning"),
                    estimatedDuration = 120
                ))
            }
        }

        // Review and refinement phase
        steps.add(WorkflowStep(
            id = "review",
            description = "Review results and provide feedback",
            assignedAgents = agents.map { it.id },
            dependencies = analysis.requiredDomains.map { "execute_$it" },
            estimatedDuration = 45
        ))

        return WorkflowPlan(steps = steps)
    }

    /**
     * Execute workflow step
     */
    private suspend fun executeWorkflowStep(
        step: WorkflowStep,
        agents: List<StatefulAgent>,
        session: CollaborationSession,
        communications: MutableList<AgentMessage>
    ): Any {
        val assignedAgents = agents.filter { step.assignedAgents.contains(it.id) }

        // Have agents collaborate on this step
        val stepResults = mutableListOf<Any>()

        for (agent in assignedAgents) {
            val context = mapOf(
                "step" to step,
                "session" to session,
                "previous_results" to stepResults,
                "shared_memory" to sharedMemory.getRelevantMemories(step.description, 5)
            )

            val result = agent.executeTask(step.description, context)

            // Agent might communicate with others
            val agentCommunications = agent.communicateWithPeers(step.description, agents, session)
            communications.addAll(agentCommunications)

            stepResults.add(result)
        }

        // Synthesize step results
        return synthesizeStepResults(stepResults, step)
    }

    /**
     * Process agent communications
     */
    private suspend fun processAgentCommunications(
        communications: List<AgentMessage>,
        agents: List<StatefulAgent>,
        session: CollaborationSession
    ) {
        communications.forEach { message ->
            val recipient = agents.find { it.id == message.recipientId }
            recipient?.receiveMessage(message)

            // Store in shared memory
            sharedMemory.storeCommunication(message, session.id)
        }
    }

    /**
     * Update agent memories and learning
     */
    private fun updateAgentMemories(
        session: CollaborationSession,
        result: CollaborativeTaskResult,
        agents: List<StatefulAgent>
    ) {
        agents.forEach { agent ->
            // Update personal memory
            agent.memory.storeExperience(
                task = session.task,
                result = result,
                context = session.context,
                collaborators = agents.map { it.id }.filter { it != agent.id }
            )

            // Update learning system
            agent.learning.updateFromExperience(
                task = session.task,
                success = result.success,
                feedback = extractFeedback(result)
            )

            // Share learnings with other agents
            shareLearnings(agent, agents, session)
        }
    }

    /**
     * Share learnings between agents
     */
    private fun shareLearnings(
        sourceAgent: StatefulAgent,
        allAgents: List<StatefulAgent>,
        session: CollaborationSession
    ) {
        val learnings = sourceAgent.learning.extractShareableKnowledge()

        allAgents.filter { it.id != sourceAgent.id }.forEach { targetAgent ->
            targetAgent.learning.receiveSharedKnowledge(learnings, sourceAgent.id)
        }

        // Store in shared memory
        sharedMemory.storeLearnings(learnings, sourceAgent.id, session.id)
    }

    /**
     * Persist agent states
     */
    private fun persistAgentStates(agents: List<StatefulAgent>) {
        agents.forEach { agent ->
            val state = agent.saveState()
            agentStates[agent.id] = state

            // Save to disk
            saveAgentStateToDisk(agent.id, state)
        }
    }

    /**
     * Load persisted states
     */
    private fun loadPersistedStates() {
        try {
            if (!Files.exists(agentStatePath)) return

            Files.list(agentStatePath)
                .filter { it.toString().endsWith(".json") }
                .forEach { path ->
                    try {
                        val content = Files.readString(path)
                        val state = json.decodeFromString<AgentState>(content)
                        agentStates[state.agentId] = state
                    } catch (e: Exception) {
                        println("Failed to load agent state from ${path.fileName}: ${e.message}")
                    }
                }
        } catch (e: Exception) {
            println("Failed to load persisted agent states: ${e.message}")
        }
    }

    /**
     * Save agent state to disk
     */
    private fun saveAgentStateToDisk(agentId: String, state: AgentState) {
        try {
            Files.createDirectories(agentStatePath)
            val stateFile = agentStatePath.resolve("$agentId.json")
            val content = json.encodeToString(AgentState.serializer(), state)
            Files.writeString(stateFile, content)
        } catch (e: Exception) {
            println("Failed to save agent state for $agentId: ${e.message}")
        }
    }

    /**
     * Get agent by ID
     */
    fun getAgent(agentId: String): StatefulAgent? = activeAgents[agentId]

    /**
     * Get all active agents
     */
    fun getAllAgents(): List<StatefulAgent> = activeAgents.values.toList()

    /**
     * Get shared memory instance
     */
    fun getSharedMemory(): SharedMemorySystem = sharedMemory

    /**
     * Shutdown the agent system
     */
    suspend fun shutdown() {
        // Persist all agent states
        activeAgents.values.forEach { agent ->
            persistAgentStates(listOf(agent))
        }

        // Shutdown communication system
        agentCommunication.shutdown()

        // Cancel coroutines
        scope.cancel()
    }

    // Helper functions
    private fun estimateComplexity(task: String): TaskComplexity {
        val keywords = listOf("complex", "advanced", "sophisticated", "multi-step", "distributed")
        val simpleKeywords = listOf("simple", "basic", "straightforward", "quick")

        return when {
            keywords.any { task.contains(it, ignoreCase = true) } -> TaskComplexity.HIGH
            simpleKeywords.any { task.contains(it, ignoreCase = true) } -> TaskComplexity.LOW
            task.length > 200 -> TaskComplexity.MEDIUM
            else -> TaskComplexity.LOW
        }
    }

    private fun identifyRequiredDomains(task: String): List<String> {
        val domains = mutableListOf<String>()

        val domainMappings = mapOf(
            "kotlin" to "kotlin",
            "java" to "java",
            "cpp" to "cpp",
            "python" to "python",
            "javascript" to "javascript",
            "test" to "testing",
            "analyze" to "static_analysis",
            "performance" to "performance_analysis",
            "security" to "security_analysis",
            "document" to "documentation",
            "debug" to "debugging",
            "architect" to "architecture",
            "design" to "architecture"
        )

        domainMappings.forEach { (keyword, domain) ->
            if (task.contains(keyword, ignoreCase = true) && domain !in domains) {
                domains.add(domain)
            }
        }

        return domains.ifEmpty { listOf("general") }
    }

    private fun identifyTaskDependencies(task: String, context: Map<String, Any>): List<String> {
        // Simple dependency analysis - could be enhanced
        return emptyList()
    }

    private fun estimateDuration(complexity: TaskComplexity, domainCount: Int): Long {
        val baseTime = when (complexity) {
            TaskComplexity.LOW -> 5
            TaskComplexity.MEDIUM -> 15
            TaskComplexity.HIGH -> 45
        }

        return baseTime * maxOf(1, domainCount / 2) * 1000L // Convert to milliseconds
    }

    private fun assessRiskLevel(task: String, context: Map<String, Any>): RiskLevel {
        val highRiskKeywords = listOf("delete", "remove", "replace", "dangerous", "critical")
        val mediumRiskKeywords = listOf("modify", "change", "update", "refactor")

        return when {
            highRiskKeywords.any { task.contains(it, ignoreCase = true) } -> RiskLevel.HIGH
            mediumRiskKeywords.any { task.contains(it, ignoreCase = true) } -> RiskLevel.MEDIUM
            else -> RiskLevel.LOW
        }
    }

    private fun calculateAgentRelevanceScore(agent: StatefulAgent, analysis: TaskAnalysis): Double {
        var score = 0.0

        // Capability match
        val capabilityMatches = analysis.requiredDomains.count { domain ->
            agent.capabilities.contains(domain)
        }
        score += capabilityMatches * 0.4

        // Role match
        val roleMatches = analysis.requiredDomains.count { domain ->
            agent.role.domains.contains(domain)
        }
        score += roleMatches * 0.3

        // Past performance (from memory)
        val pastPerformance = agent.memory.getAveragePerformanceScore()
        score += pastPerformance * 0.3

        return score
    }

    private fun synthesizeStepResults(results: List<Any>, step: WorkflowStep): Any {
        // Simple synthesis - combine results
        return mapOf(
            "step" to step.id,
            "results" to results,
            "synthesized" to results.joinToString("\n")
        )
    }

    private fun synthesizeResults(
        results: Map<String, Any>,
        analysis: TaskAnalysis,
        agents: List<StatefulAgent>
    ): Any {
        return mapOf(
            "task" to analysis.task,
            "results" to results,
            "agents_used" to agents.map { it.id },
            "final_output" to generateFinalOutput(results, analysis)
        )
    }

    private fun generateFinalOutput(results: Map<String, Any>, analysis: TaskAnalysis): String {
        val output = StringBuilder()

        output.append("🤖 Multi-Agent Task Completion Report\n")
        output.append("=====================================\n\n")

        output.append("📋 Task: ${analysis.task}\n")
        output.append("🎯 Complexity: ${analysis.complexity}\n")
        output.append("🏷️ Domains: ${analysis.requiredDomains.joinToString(", ")}\n\n")

        results.forEach { (step, result) ->
            output.append("✅ $step: ${extractResultSummary(result)}\n")
        }

        output.append("\n🎉 Task completed successfully!")

        return output.toString()
    }

    private fun extractResultSummary(result: Any): String {
        return when (result) {
            is Map<*, *> -> result["synthesized"]?.toString() ?: "Completed"
            is String -> if (result.length > 50) result.take(50) + "..." else result
            else -> "Completed"
        }
    }

    private fun extractFeedback(result: CollaborativeTaskResult): Map<String, Any> {
        return mapOf(
            "success" to result.success,
            "quality_score" to 0.85, // Could be calculated based on various metrics
            "collaboration_effectiveness" to calculateCollaborationEffectiveness(result)
        )
    }

    private fun calculateCollaborationEffectiveness(result: CollaborativeTaskResult): Double {
        // Simple effectiveness calculation
        val agentCount = result.participatingAgents.size
        val communicationEfficiency = minOf(1.0, result.communications.toDouble() / (agentCount * 5))
        val workflowEfficiency = minOf(1.0, result.workflowSteps.toDouble() / 10)

        return (communicationEfficiency + workflowEfficiency) / 2.0
    }

    private fun calculateCollaborationMetrics(
        session: CollaborationSession,
        result: CollaborativeTaskResult
    ): CollaborationMetrics {
        return CollaborationMetrics(
            totalAgents = result.participatingAgents.size,
            totalCommunications = result.communications,
            workflowSteps = result.workflowSteps,
            averageAgentContribution = calculateAverageContribution(result),
            collaborationEfficiency = calculateCollaborationEffectiveness(result)
        )
    }

    private fun calculateAverageContribution(result: CollaborativeTaskResult): Double {
        return if (result.participatingAgents.isNotEmpty()) {
            1.0 / result.participatingAgents.size
        } else {
            0.0
        }
    }

    private fun ensureDirectoriesExist() {
        try {
            Files.createDirectories(agentStatePath)
            Files.createDirectories(sharedMemoryPath)
        } catch (e: Exception) {
            println("Failed to create agent directories: ${e.message}")
        }
    }
}

// Data Classes
@Serializable
data class AgentState(
    val agentId: String,
    val memory: AgentMemoryData,
    val learning: LearningData,
    val lastActive: Long,
    val totalTasks: Int,
    val successRate: Double
)

@Serializable
data class AgentMemoryData(
    val experiences: List<Experience> = emptyList(),
    val preferences: Map<String, Any> = emptyMap(),
    val contextPatterns: List<ContextPattern> = emptyList()
)

@Serializable
data class LearningData(
    val patterns: List<LearnedPattern> = emptyList(),
    val feedbackHistory: List<FeedbackEntry> = emptyList(),
    val adaptationRules: List<AdaptationRule> = emptyList()
)

@Serializable
data class Experience(
    val task: String,
    val result: String,
    val success: Boolean,
    val timestamp: Long,
    val collaborators: List<String> = emptyList(),
    val context: Map<String, Any> = emptyMap()
)

@Serializable
data class ContextPattern(
    val pattern: String,
    val frequency: Int,
    val successRate: Double,
    val associatedActions: List<String> = emptyList()
)

@Serializable
data class LearnedPattern(
    val pattern: String,
    val confidence: Double,
    val applicationCount: Int,
    val successRate: Double
)

@Serializable
data class FeedbackEntry(
    val task: String,
    val rating: Double,
    val comments: String,
    val timestamp: Long
)

@Serializable
data class AdaptationRule(
    val condition: String,
    val action: String,
    val confidence: Double,
    val applicationCount: Int
)

enum class AgentRole(val domains: List<String>) {
    CODE_GENERATION(listOf("kotlin", "java", "cpp", "python", "javascript")),
    CODE_ANALYSIS(listOf("static_analysis", "performance_analysis", "security_analysis")),
    ARCHITECTURE(listOf("design_patterns", "system_architecture", "scalability")),
    TESTING(listOf("unit_tests", "integration_tests", "performance_tests")),
    DOCUMENTATION(listOf("api_docs", "code_comments", "readme_generation")),
    DEBUGGING(listOf("error_analysis", "bug_fixing", "performance_debugging")),
    LEARNING(listOf("pattern_recognition", "feedback_analysis", "optimization"))
}

enum class AgentPersonality {
    CONSERVATIVE,  // Prefers safe, proven approaches
    INNOVATIVE,    // Takes risks, tries new approaches
    BALANCED,      // Mix of conservative and innovative
    SPECIALIZED,   // Deep expertise in specific areas
    GENERALIST     // Broad knowledge across domains
}

enum class TaskComplexity {
    LOW, MEDIUM, HIGH
}

enum class RiskLevel {
    LOW, MEDIUM, HIGH
}

@Serializable
data class TaskAnalysis(
    val task: String,
    val complexity: TaskComplexity,
    val requiredDomains: List<String>,
    val dependencies: List<String>,
    val estimatedDuration: Long,
    val riskLevel: RiskLevel
)

@Serializable
data class CollaborationSession(
    val id: String,
    val task: String,
    val context: Map<String, Any>,
    val participants: List<String>,
    val startTime: Long
)

@Serializable
data class WorkflowPlan(
    val steps: List<WorkflowStep>
)

@Serializable
data class WorkflowStep(
    val id: String,
    val description: String,
    val assignedAgents: List<String>,
    val dependencies: List<String>,
    val estimatedDuration: Int // in seconds
)

@Serializable
data class CollaborativeTaskResult(
    val sessionId: String,
    val success: Boolean,
    val result: Any,
    val participatingAgents: List<String>,
    val workflowSteps: Int,
    val communications: Int,
    val executionTime: Long = 0,
    val collaborationMetrics: CollaborationMetrics = CollaborationMetrics()
)

@Serializable
data class CollaborationMetrics(
    val totalAgents: Int = 0,
    val totalCommunications: Int = 0,
    val workflowSteps: Int = 0,
    val averageAgentContribution: Double = 0.0,
    val collaborationEfficiency: Double = 0.0
)

// Stateful Agent Implementation
class StatefulAgent(
    val id: String,
    val role: AgentRole,
    val capabilities: List<String>,
    val personality: AgentPersonality = AgentPersonality.BALANCED,
    val memory: AgentMemory = AgentMemory(),
    val learning: LearningSystem = LearningSystem(),
    val communication: AgentCommunication
) {

    suspend fun executeTask(task: String, context: Map<String, Any> = emptyMap()): Any {
        // Use memory and learning to inform execution
        val relevantMemories = memory.getRelevantMemories(task, 3)
        val learnedPatterns = learning.getApplicablePatterns(task)
        val sharedKnowledge = communication.getSharedKnowledge(task)

        // Combine context with agent knowledge
        val enhancedContext = context + mapOf(
            "memories" to relevantMemories,
            "patterns" to learnedPatterns,
            "shared_knowledge" to sharedKnowledge,
            "personality" to personality,
            "role" to role
        )

        // Execute task (delegate to specific implementation)
        val result = executeTaskImplementation(task, enhancedContext)

        // Update memory
        memory.storeExecution(task, result, context)

        return result
    }

    private fun executeTaskImplementation(task: String, context: Map<String, Any>): Any {
        // This would delegate to specific agent implementations
        // For now, return a basic result structure
        return mapOf(
            "task" to task,
            "result" to "Task executed by ${id} (${role})",
            "confidence" to 0.8,
            "context_used" to context.keys
        )
    }

    suspend fun communicateWithPeers(
        topic: String,
        peers: List<StatefulAgent>,
        session: CollaborationSession
    ): List<AgentMessage> {
        val messages = mutableListOf<AgentMessage>()

        // Generate relevant communications based on role and capabilities
        peers.filter { it.id != id }.forEach { peer ->
            if (shouldCommunicateWith(peer, topic)) {
                val message = AgentMessage(
                    senderId = id,
                    recipientId = peer.id,
                    content = generateCommunicationContent(topic, peer),
                    type = MessageType.COLLABORATION,
                    sessionId = session.id,
                    timestamp = System.currentTimeMillis()
                )
                messages.add(message)
            }
        }

        return messages
    }

    fun receiveMessage(message: AgentMessage) {
        // Process incoming message
        memory.storeCommunication(message)

        // Potentially update learning based on message
        if (message.type == MessageType.FEEDBACK) {
            learning.processFeedback(message.content)
        }
    }

    fun saveState(): AgentState {
        return AgentState(
            agentId = id,
            memory = memory.saveState(),
            learning = learning.saveState(),
            lastActive = System.currentTimeMillis(),
            totalTasks = memory.getTotalExecutions(),
            successRate = memory.getSuccessRate()
        )
    }

    fun loadState(state: AgentState) {
        memory.loadState(state.memory)
        learning.loadState(state.learning)
    }

    private fun shouldCommunicateWith(peer: StatefulAgent, topic: String): Boolean {
        // Determine if communication is beneficial
        return capabilities.any { cap -> peer.capabilities.contains(cap) } ||
               role.domains.any { domain -> peer.role.domains.contains(domain) }
    }

    private fun generateCommunicationContent(topic: String, peer: StatefulAgent): String {
        return "Agent $id sharing insights about '$topic' with ${peer.id}"
    }
}

// Supporting Classes
class AgentMemory {
    private val experiences = mutableListOf<Experience>()
    private val preferences = mutableMapOf<String, Any>()
    private val contextPatterns = mutableListOf<ContextPattern>()

    fun storeExperience(task: String, result: Any, context: Map<String, Any>, collaborators: List<String>) {
        val experience = Experience(
            task = task,
            result = result.toString(),
            success = true, // Could be determined from result
            timestamp = System.currentTimeMillis(),
            collaborators = collaborators,
            context = context
        )
        experiences.add(experience)

        // Update patterns
        updateContextPatterns(experience)
    }

    fun storeExecution(task: String, result: Any, context: Map<String, Any>) {
        storeExperience(task, result, context, emptyList())
    }

    fun storeCommunication(message: AgentMessage) {
        // Store communication for future reference
    }

    fun getRelevantMemories(query: String, limit: Int): List<Experience> {
        return experiences
            .filter { it.task.contains(query, ignoreCase = true) }
            .sortedByDescending { it.timestamp }
            .take(limit)
    }

    fun getAveragePerformanceScore(): Double {
        if (experiences.isEmpty()) return 0.5
        return experiences.count { it.success }.toDouble() / experiences.size
    }

    fun getTotalExecutions(): Int = experiences.size

    fun getSuccessRate(): Double = getAveragePerformanceScore()

    fun saveState(): AgentMemoryData {
        return AgentMemoryData(
            experiences = experiences.takeLast(100), // Keep last 100 experiences
            preferences = preferences,
            contextPatterns = contextPatterns
        )
    }

    fun loadState(data: AgentMemoryData) {
        experiences.clear()
        experiences.addAll(data.experiences)
        preferences.putAll(data.preferences)
        contextPatterns.clear()
        contextPatterns.addAll(data.contextPatterns)
    }

    private fun updateContextPatterns(experience: Experience) {
        // Simple pattern recognition
        val contextKey = experience.context.entries
            .sortedBy { it.key }
            .joinToString("|") { "${it.key}:${it.value}" }

        val existingPattern = contextPatterns.find { it.pattern == contextKey }
        if (existingPattern != null) {
            existingPattern.frequency++
            if (experience.success) {
                existingPattern.successRate =
                    (existingPattern.successRate * (existingPattern.frequency - 1) + 1) / existingPattern.frequency
            }
        } else {
            contextPatterns.add(ContextPattern(
                pattern = contextKey,
                frequency = 1,
                successRate = if (experience.success) 1.0 else 0.0
            ))
        }
    }
}

class LearningSystem {
    private val patterns = mutableListOf<LearnedPattern>()
    private val feedbackHistory = mutableListOf<FeedbackEntry>()
    private val adaptationRules = mutableListOf<AdaptationRule>()

    fun updateFromExperience(task: String, success: Boolean, feedback: Map<String, Any>) {
        val rating = feedback["quality_score"] as? Double ?: (if (success) 0.8 else 0.2)

        feedbackHistory.add(FeedbackEntry(
            task = task,
            rating = rating,
            comments = feedback.toString(),
            timestamp = System.currentTimeMillis()
        ))

        // Update patterns
        updatePatterns(task, success, rating)
    }

    fun getApplicablePatterns(task: String): List<LearnedPattern> {
        return patterns.filter { pattern ->
            task.contains(pattern.pattern, ignoreCase = true)
        }.sortedByDescending { it.confidence }
    }

    fun processFeedback(feedback: String) {
        // Process feedback to improve future performance
    }

    fun extractShareableKnowledge(): Map<String, Any> {
        return mapOf(
            "successful_patterns" to patterns.filter { it.successRate > 0.8 },
            "adaptation_rules" to adaptationRules.filter { it.confidence > 0.7 }
        )
    }

    fun receiveSharedKnowledge(knowledge: Map<String, Any>, sourceAgent: String) {
        // Integrate knowledge from other agents
        val sharedPatterns = knowledge["successful_patterns"] as? List<*> ?: emptyList()
        val sharedRules = knowledge["adaptation_rules"] as? List<*> ?: emptyList()

        // Add new patterns with lower initial confidence
        sharedPatterns.forEach { pattern ->
            if (pattern is LearnedPattern) {
                patterns.add(pattern.copy(
                    confidence = pattern.confidence * 0.8, // Reduce confidence for shared knowledge
                    applicationCount = 0
                ))
            }
        }
    }

    fun saveState(): LearningData {
        return LearningData(
            patterns = patterns,
            feedbackHistory = feedbackHistory.takeLast(50), // Keep last 50 feedback entries
            adaptationRules = adaptationRules
        )
    }

    fun loadState(data: LearningData) {
        patterns.clear()
        patterns.addAll(data.patterns)
        feedbackHistory.clear()
        feedbackHistory.addAll(data.feedbackHistory)
        adaptationRules.clear()
        adaptationRules.addAll(data.adaptationRules)
    }

    private fun updatePatterns(task: String, success: Boolean, rating: Double) {
        // Extract patterns from successful tasks
        if (success && rating > 0.7) {
            val keywords = extractKeywords(task)
            keywords.forEach { keyword ->
                val existingPattern = patterns.find { it.pattern == keyword }
                if (existingPattern != null) {
                    existingPattern.applicationCount++
                    existingPattern.successRate =
                        (existingPattern.successRate * (existingPattern.applicationCount - 1) + rating) /
                        existingPattern.applicationCount
                    existingPattern.confidence = minOf(1.0, existingPattern.confidence + 0.1)
                } else {
                    patterns.add(LearnedPattern(
                        pattern = keyword,
                        confidence = 0.5,
                        applicationCount = 1,
                        successRate = rating
                    ))
                }
            }
        }
    }

    private fun extractKeywords(text: String): List<String> {
        // Simple keyword extraction
        return text.lowercase()
            .split(Regex("\\W+"))
            .filter { it.length > 3 }
            .distinct()
    }
}

class AgentCommunication(
    private val agentId: String,
    private val communicationSystem: AgentCommunicationSystem
) {
    suspend fun sendMessage(recipientId: String, content: String, type: MessageType, sessionId: String): AgentMessage {
        val message = AgentMessage(
            senderId = agentId,
            recipientId = recipientId,
            content = content,
            type = type,
            sessionId = sessionId,
            timestamp = System.currentTimeMillis()
        )

        communicationSystem.sendMessage(message)
        return message
    }

    fun getSharedKnowledge(query: String): List<AgentMessage> {
        return communicationSystem.getSharedKnowledge(agentId, query)
    }
}

class AgentCommunicationSystem {
    private val messageHistory = ConcurrentHashMap<String, MutableList<AgentMessage>>()
    private val activeAgents = ConcurrentHashMap<String, StatefulAgent>()

    fun registerAgent(agent: StatefulAgent) {
        activeAgents[agent.id] = agent
        messageHistory[agent.id] = mutableListOf()
    }

    suspend fun sendMessage(message: AgentMessage) {
        // Store message
        messageHistory.getOrPut(message.recipientId) { mutableListOf() }.add(message)

        // Deliver to recipient if active
        val recipient = activeAgents[message.recipientId]
        recipient?.receiveMessage(message)
    }

    fun getSharedKnowledge(agentId: String, query: String): List<AgentMessage> {
        return messageHistory[agentId]?.filter { message ->
            message.content.contains(query, ignoreCase = true)
        } ?: emptyList()
    }

    fun getConversationHistory(agent1: String, agent2: String): List<AgentMessage> {
        val messages = mutableListOf<AgentMessage>()

        messageHistory[agent1]?.filter { it.recipientId == agent2 }?.let { messages.addAll(it) }
        messageHistory[agent2]?.filter { it.recipientId == agent1 }?.let { messages.addAll(it) }

        return messages.sortedBy { it.timestamp }
    }

    suspend fun shutdown() {
        activeAgents.clear()
        messageHistory.clear()
    }
}

class SharedMemorySystem {
    private val memories = ConcurrentHashMap<String, MutableList<MemoryEntry>>()

    fun storeCommunication(message: AgentMessage, sessionId: String) {
        val entry = MemoryEntry(
            type = MemoryType.COMMUNICATION,
            content = message.content,
            metadata = mapOf(
                "sender" to message.senderId,
                "recipient" to message.recipientId,
                "session" to sessionId,
                "timestamp" to message.timestamp
            ),
            timestamp = message.timestamp
        )

        memories.getOrPut("communications") { mutableListOf() }.add(entry)
    }

    fun storeLearnings(learnings: Map<String, Any>, sourceAgent: String, sessionId: String) {
        val entry = MemoryEntry(
            type = MemoryType.LEARNING,
            content = learnings.toString(),
            metadata = mapOf(
                "source_agent" to sourceAgent,
                "session" to sessionId,
                "timestamp" to System.currentTimeMillis()
            ),
            timestamp = System.currentTimeMillis()
        )

        memories.getOrPut("learnings") { mutableListOf() }.add(entry)
    }

    fun getRelevantMemories(query: String, limit: Int): List<MemoryEntry> {
        val allMemories = memories.values.flatten()
        return allMemories
            .filter { it.content.contains(query, ignoreCase = true) }
            .sortedByDescending { it.timestamp }
            .take(limit)
    }

    fun getMemoriesByType(type: MemoryType): List<MemoryEntry> {
        return memories[type.name.lowercase()] ?: emptyList()
    }

    fun cleanupOldMemories(maxAge: Long = 30 * 24 * 60 * 60 * 1000L) { // 30 days
        val cutoff = System.currentTimeMillis() - maxAge
        memories.values.forEach { list ->
            list.removeIf { it.timestamp < cutoff }
        }
    }
}

enum class MemoryType {
    COMMUNICATION, LEARNING, EXPERIENCE, PATTERN
}

@Serializable
data class MemoryEntry(
    val type: MemoryType,
    val content: String,
    val metadata: Map<String, Any> = emptyMap(),
    val timestamp: Long
)

enum class MessageType {
    COLLABORATION, FEEDBACK, REQUEST, RESPONSE, BROADCAST
}

@Serializable
data class AgentMessage(
    val senderId: String,
    val recipientId: String,
    val content: String,
    val type: MessageType,
    val sessionId: String,
    val timestamp: Long
)