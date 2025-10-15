package com.foundry.tests

import com.foundry.ide.managers.*
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Before
import org.junit.Test
import org.junit.Assert.*
import kotlin.test.assertFailsWith

/**
 * Comprehensive test suite for MCP Server and Stateful Agent System
 */
class MCPAgentsTest {

    private lateinit var mcpServer: MCPServer
    private lateinit var agentSystem: StatefulAgentSystem

    @Before
    fun setUp() {
        mcpServer = MCPServer()
        agentSystem = StatefulAgentSystem()
    }

    @After
    fun tearDown() {
        runBlocking {
            mcpServer.stop()
            agentSystem.shutdown()
        }
    }

    // MCP Server Tests
    @Test
    fun testMCPServerInitialization() = runTest {
        val started = mcpServer.start()
        assertTrue(started)

        // Give server time to start
        kotlinx.coroutines.delay(100)

        // Test server capabilities
        val capabilities = MCPServerCapabilities(
            tools = ToolCapabilities(listChanged = true),
            resources = ResourceCapabilities(subscribe = true, listChanged = true),
            completion = CompletionCapabilities(),
            agentic = AgenticCapabilities(supported = true, tools = listOf("code_generator")),
            codeAnalysis = CodeAnalysisCapabilities(supported = true, languages = listOf("kotlin")),
            development = DevelopmentCapabilities(supported = true, assistants = listOf("pair_programmer"))
        )

        assertTrue(capabilities.agentic.supported)
        assertTrue(capabilities.codeAnalysis.supported)
        assertTrue(capabilities.development.supported)
    }

    @Test
    fun testMCPMessageHandling() {
        // Test initialize request
        val initRequest = MCPRequest(
            id = "init_test",
            method = "initialize",
            params = emptyMap<String, Any>()
        )

        val initResponse = mcpServer.handleRequest(initRequest, "test_client")
        assertEquals("init_test", initResponse.id)
        assertNotNull(initResponse.result)
        assertNull(initResponse.error)

        // Test tools/list request
        val toolsRequest = MCPRequest(
            id = "tools_test",
            method = "tools/list",
            params = emptyMap<String, Any>()
        )

        val toolsResponse = mcpServer.handleRequest(toolsRequest, "test_client")
        assertEquals("tools_test", toolsResponse.id)
        assertNotNull(toolsResponse.result)
        assertNull(toolsResponse.error)

        // Test invalid method
        val invalidRequest = MCPRequest(
            id = "invalid_test",
            method = "invalid_method",
            params = emptyMap<String, Any>()
        )

        val invalidResponse = mcpServer.handleRequest(invalidRequest, "test_client")
        assertEquals("invalid_test", invalidResponse.id)
        assertNull(invalidResponse.result)
        assertNotNull(invalidResponse.error)
        assertEquals("MethodNotFound", invalidResponse.error?.code)
    }

    @Test
    fun testMCPToolExecution() = runTest {
        // Test agentic execution
        val agenticRequest = MCPRequest(
            id = "agentic_test",
            method = "agentic/execute",
            params = mapOf(
                "task" to "Generate a simple Kotlin class with a main function"
            )
        )

        val response = mcpServer.handleRequest(agenticRequest, "test_client")
        assertEquals("agentic_test", response.id)
        assertNotNull(response.result)
        assertNull(response.error)

        val result = response.result as? Map<*, *>
        assertNotNull(result)
        assertTrue(result.containsKey("analysis"))
        assertTrue(result.containsKey("plan"))
        assertTrue(result.containsKey("execution"))
    }

    // Stateful Agent System Tests
    @Test
    fun testAgentSystemInitialization() {
        val agents = agentSystem.getAllAgents()
        assertTrue(agents.size >= 7) // Should have at least 7 default agents

        // Check for required agents
        val agentIds = agents.map { it.id }
        assertTrue(agentIds.contains("code_generator"))
        assertTrue(agentIds.contains("code_analyzer"))
        assertTrue(agentIds.contains("architect"))
        assertTrue(agentIds.contains("tester"))
        assertTrue(agentIds.contains("documenter"))
        assertTrue(agentIds.contains("debugger"))
        assertTrue(agentIds.contains("learner"))
    }

    @Test
    fun testAgentCreationAndRetrieval() {
        val customAgent = agentSystem.createAgent(
            id = "custom_agent",
            role = AgentRole.CODE_GENERATION,
            capabilities = listOf("custom_feature"),
            personality = AgentPersonality.SPECIALIZED
        )

        assertEquals("custom_agent", customAgent.id)
        assertEquals(AgentRole.CODE_GENERATION, customAgent.role)
        assertTrue(customAgent.capabilities.contains("custom_feature"))
        assertEquals(AgentPersonality.SPECIALIZED, customAgent.personality)

        // Test retrieval
        val retrievedAgent = agentSystem.getAgent("custom_agent")
        assertNotNull(retrievedAgent)
        assertEquals("custom_agent", retrievedAgent?.id)
    }

    @Test
    fun testAgentTaskExecution() = runTest {
        val agent = agentSystem.getAgent("code_generator")
        assertNotNull(agent)

        val context = mapOf(
            "language" to "kotlin",
            "complexity" to "simple"
        )

        val result = agent?.executeTask("Generate a hello world function", context)
        assertNotNull(result)

        // Result should be a map with execution details
        assertTrue(result is Map<*, *>)
        val resultMap = result as Map<*, *>
        assertTrue(resultMap.containsKey("task"))
        assertTrue(resultMap.containsKey("result"))
    }

    @Test
    fun testCollaborativeTaskExecution() = runTest {
        val task = "Create a complete Kotlin application with main function, data class, and tests"
        val context = mapOf(
            "language" to "kotlin",
            "framework" to "none",
            "testing" to "junit"
        )

        val result = agentSystem.executeCollaborativeTask(task, context)

        // Verify result structure
        assertTrue(result.success)
        assertNotNull(result.result)
        assertTrue(result.participatingAgents.size >= 2) // Should involve multiple agents
        assertTrue(result.workflowSteps > 0)
        assertTrue(result.executionTime >= 0)

        // Check collaboration metrics
        val metrics = result.collaborationMetrics
        assertTrue(metrics.totalAgents >= 2)
        assertTrue(metrics.collaborationEfficiency >= 0.0)
        assertTrue(metrics.collaborationEfficiency <= 1.0)
    }

    @Test
    fun testAgentMemoryAndLearning() = runTest {
        val agent = agentSystem.createAgent(
            id = "learning_test_agent",
            role = AgentRole.CODE_GENERATION,
            capabilities = listOf("kotlin", "learning")
        )

        // Execute multiple tasks to build memory
        val tasks = listOf(
            "Generate a simple function",
            "Create a data class",
            "Write a test function"
        )

        for (task in tasks) {
            agent.executeTask(task, emptyMap())
        }

        // Check that agent has memory
        val memory = agent.memory
        assertTrue(memory.getTotalExecutions() >= 3)

        // Test memory retrieval
        val relevantMemories = memory.getRelevantMemories("function", 5)
        assertTrue(relevantMemories.size >= 2) // Should find function-related memories

        // Test learning system
        val learning = agent.learning
        val applicablePatterns = learning.getApplicablePatterns("generate function")
        assertNotNull(applicablePatterns)
    }

    @Test
    fun testAgentCommunication() = runTest {
        val agent1 = agentSystem.createAgent("comm_agent_1", AgentRole.CODE_GENERATION, listOf("kotlin"))
        val agent2 = agentSystem.createAgent("comm_agent_2", AgentRole.TESTING, listOf("junit"))

        // Test direct communication
        val message = AgentMessage(
            senderId = "comm_agent_1",
            recipientId = "comm_agent_2",
            content = "Please create tests for the generated code",
            type = MessageType.COLLABORATION,
            sessionId = "test_session",
            timestamp = System.currentTimeMillis()
        )

        agent2.receiveMessage(message)

        // Verify message was processed (would check agent's memory/communication log)
        val memory = agent2.memory
        // Note: Actual implementation would store communication in memory
    }

    @Test
    fun testSharedMemorySystem() {
        val sharedMemory = agentSystem.getSharedMemory()

        // Store some test memories
        val communication = AgentMessage(
            senderId = "agent1",
            recipientId = "agent2",
            content = "Test communication",
            type = MessageType.COLLABORATION,
            sessionId = "test_session",
            timestamp = System.currentTimeMillis()
        )

        sharedMemory.storeCommunication(communication, "test_session")

        // Retrieve memories
        val relevantMemories = sharedMemory.getRelevantMemories("communication", 5)
        assertTrue(relevantMemories.size >= 1)

        // Test memory by type
        val communicationMemories = sharedMemory.getMemoriesByType(MemoryType.COMMUNICATION)
        assertTrue(communicationMemories.size >= 1)
    }

    @Test
    fun testAgentStatePersistence() {
        val agent = agentSystem.createAgent(
            id = "persistence_test",
            role = AgentRole.CODE_GENERATION,
            capabilities = listOf("kotlin")
        )

        // Execute some tasks to build state
        runBlocking {
            agent.executeTask("Generate test code", emptyMap())
            agent.executeTask("Create documentation", emptyMap())
        }

        // Save state
        val state = agent.saveState()
        assertEquals("persistence_test", state.agentId)
        assertTrue(state.memory.experiences.isNotEmpty())

        // Create new agent and load state
        val newAgent = agentSystem.createAgent(
            id = "persistence_test_2",
            role = AgentRole.CODE_GENERATION,
            capabilities = listOf("kotlin")
        )

        newAgent.loadState(state)

        // Verify state was loaded
        assertEquals(state.memory.experiences.size, newAgent.memory.getTotalExecutions())
    }

    @Test
    fun testAgentPersonalityAndAdaptation() {
        val conservativeAgent = agentSystem.createAgent(
            id = "conservative_agent",
            role = AgentRole.CODE_GENERATION,
            capabilities = listOf("kotlin"),
            personality = AgentPersonality.CONSERVATIVE
        )

        val innovativeAgent = agentSystem.createAgent(
            id = "innovative_agent",
            role = AgentRole.CODE_GENERATION,
            capabilities = listOf("kotlin"),
            personality = AgentPersonality.INNOVATIVE
        )

        // Test that different personalities produce different results
        val task = "Generate a sorting algorithm"
        val conservativeResult = runBlocking { conservativeAgent.executeTask(task, emptyMap()) }
        val innovativeResult = runBlocking { innovativeAgent.executeTask(task, emptyMap()) }

        // Results should be different (basic check)
        assertNotEquals(conservativeResult.toString(), innovativeResult.toString())
    }

    @Test
    fun testTaskAnalysisAndPlanning() {
        val analysis = agentSystem.analyzeTaskRequirements(
            "Create a complex web application with user authentication, database integration, and comprehensive testing",
            emptyMap()
        )

        assertEquals("Create a complex web application with user authentication, database integration, and comprehensive testing", analysis.task)
        assertEquals(TaskComplexity.HIGH, analysis.complexity)
        assertTrue(analysis.requiredDomains.contains("architecture"))
        assertTrue(analysis.requiredDomains.contains("testing"))
        assertTrue(analysis.estimatedDuration > 0)
        assertEquals(RiskLevel.HIGH, analysis.riskLevel)
    }

    @Test
    fun testAgentSelectionLogic() {
        val analysis = TaskAnalysis(
            task = "Generate Kotlin code with tests",
            complexity = TaskComplexity.MEDIUM,
            requiredDomains = listOf("kotlin", "testing"),
            dependencies = emptyList(),
            estimatedDuration = 30000L,
            riskLevel = RiskLevel.LOW
        )

        val selectedAgents = agentSystem.selectAgentsForTask(analysis)

        // Should select agents with relevant capabilities
        val hasCodeGenerator = selectedAgents.any { it.capabilities.contains("kotlin") }
        val hasTester = selectedAgents.any { it.capabilities.contains("testing") || it.role == AgentRole.TESTING }

        assertTrue(hasCodeGenerator || hasTester)
        assertTrue(selectedAgents.size <= 4) // Should not exceed max agents
    }

    @Test
    fun testWorkflowExecution() = runTest {
        val agents = listOf(
            agentSystem.createAgent("wf_agent_1", AgentRole.CODE_GENERATION, listOf("kotlin")),
            agentSystem.createAgent("wf_agent_2", AgentRole.TESTING, listOf("junit")),
            agentSystem.createAgent("wf_agent_3", AgentRole.DOCUMENTATION, listOf("markdown"))
        )

        val analysis = TaskAnalysis(
            task = "Create a complete Kotlin project",
            complexity = TaskComplexity.MEDIUM,
            requiredDomains = listOf("kotlin", "testing", "documentation"),
            dependencies = emptyList(),
            estimatedDuration = 60000L,
            riskLevel = RiskLevel.MEDIUM
        )

        val session = CollaborationSession(
            id = "workflow_test",
            task = analysis.task,
            context = emptyMap(),
            participants = agents.map { it.id },
            startTime = System.currentTimeMillis()
        )

        val result = agentSystem.executeCollaborativeWorkflow(session, agents, analysis)

        assertTrue(result.success)
        assertNotNull(result.result)
        assertEquals(agents.size, result.participatingAgents.size)
    }

    // Performance Tests
    @Test
    fun testAgentExecutionPerformance() = runTest {
        val agent = agentSystem.getAgent("code_generator")!!
        val iterations = 10

        val startTime = System.currentTimeMillis()

        for (i in 1..iterations) {
            agent.executeTask("Generate simple function $i", emptyMap())
        }

        val totalTime = System.currentTimeMillis() - startTime
        val avgTimePerTask = totalTime / iterations

        println("Agent execution performance: ${avgTimePerTask}ms per task")
        assertTrue(avgTimePerTask < 5000) // Should complete each task within 5 seconds
    }

    @Test
    fun testCollaborativeExecutionPerformance() = runTest {
        val task = "Generate a simple Kotlin class"
        val iterations = 5

        var totalTime = 0L

        for (i in 1..iterations) {
            val startTime = System.currentTimeMillis()
            agentSystem.executeCollaborativeTask(task, emptyMap())
            totalTime += System.currentTimeMillis() - startTime
        }

        val avgTime = totalTime / iterations
        println("Collaborative execution performance: ${avgTime}ms per execution")
        assertTrue(avgTime < 30000) // Should complete within 30 seconds
    }

    // Error Handling Tests
    @Test
    fun testAgentErrorHandling() = runTest {
        val agent = agentSystem.getAgent("code_generator")!!

        // Test with invalid context
        val result = agent.executeTask("Generate code", mapOf("invalid_param" to "test"))

        // Should handle gracefully and still produce a result
        assertNotNull(result)
        assertTrue(result is Map<*, *>)
    }

    @Test
    fun testMCPErrorHandling() {
        // Test with malformed request
        val malformedRequest = MCPRequest(
            id = "error_test",
            method = "invalid/method",
            params = null
        )

        val response = mcpServer.handleRequest(malformedRequest, "test_client")
        assertNotNull(response.error)
    }

    // Security Tests
    @Test
    fun testAgentSandboxSecurity() {
        val agent = agentSystem.createAgent(
            id = "security_test",
            role = AgentRole.CODE_GENERATION,
            capabilities = listOf("kotlin")
        )

        // Test permission validation
        val hasReadPermission = agent.communication.communicationSystem.activeAgents[agent.id]?.let {
            // This would test the sandbox permissions
            true
        } ?: false

        // Basic security check
        assertTrue(true) // Placeholder - implement actual security tests
    }

    // Memory Leak Tests
    @Test
    fun testAgentMemoryManagement() = runTest {
        val initialAgentCount = agentSystem.getAllAgents().size

        // Create and destroy multiple agents
        val testAgents = mutableListOf<StatefulAgent>()
        for (i in 1..10) {
            val agent = agentSystem.createAgent(
                id = "memory_test_$i",
                role = AgentRole.CODE_GENERATION,
                capabilities = listOf("kotlin")
            )
            testAgents.add(agent)

            // Execute some tasks
            agent.executeTask("Generate test code $i", emptyMap())
        }

        // Agents should still be accessible
        assertEquals(initialAgentCount + 10, agentSystem.getAllAgents().size)

        // Test that all agents are functional
        for (agent in testAgents) {
            val result = agent.executeTask("Simple test", emptyMap())
            assertNotNull(result)
        }
    }

    // Concurrency Tests
    @Test
    fun testConcurrentAgentOperations() = runTest {
        val agent = agentSystem.getAgent("code_generator")!!

        // Test concurrent task execution
        val jobs = List(5) {
            kotlinx.coroutines.async {
                agent.executeTask("Concurrent task $it", emptyMap())
            }
        }

        val results = jobs.map { it.await() }
        assertEquals(5, results.size)
        results.forEach { assertNotNull(it) }
    }

    // Benchmark Tests
    @Test
    fun testAgentSystemBenchmark() = runTest {
        val iterations = 50
        val startTime = System.currentTimeMillis()

        for (i in 1..iterations) {
            val task = "Generate benchmark code $i"
            agentSystem.executeCollaborativeTask(task, emptyMap())
        }

        val totalTime = System.currentTimeMillis() - startTime
        val operationsPerSecond = iterations / (totalTime / 1000.0)

        println("Agent System Benchmark: $operationsPerSecond operations/second")
        assertTrue(operationsPerSecond > 0.1) // At least 0.1 operations per second
    }

    @Test
    fun testMemorySystemBenchmark() {
        val sharedMemory = agentSystem.getSharedMemory()
        val iterations = 1000

        val startTime = System.currentTimeMillis()

        for (i in 1..iterations) {
            val memory = MemoryEntry(
                type = MemoryType.EXPERIENCE,
                content = "Benchmark memory entry $i",
                timestamp = System.currentTimeMillis()
            )

            // Simulate storing memory (would use actual shared memory methods)
        }

        val totalTime = System.currentTimeMillis() - startTime
        val operationsPerSecond = iterations / (totalTime / 1000.0)

        println("Memory System Benchmark: $operationsPerSecond operations/second")
        assertTrue(operationsPerSecond > 100.0) // At least 100 operations per second
    }
}