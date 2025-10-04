import com.foundry.engine.*
import kotlinx.coroutines.*

/**
 * Sample Game Manager Script
 * Demonstrates basic game loop and entity management
 */
class GameManager : ScriptComponent() {
    private var gameStartTime = 0.0
    private var entityCount = 0
    private val scope = CoroutineScope(Dispatchers.Default)

    override fun onInitialize() {
        println("GameManager: Initializing game...")
        gameStartTime = Time.time

        // Create initial game objects
        createGameObjects()
    }

    override fun onUpdate(deltaTime: Float) {
        // Update game logic
        updateGameState(deltaTime)

        // Check for game events
        handleGameEvents()
    }

    override fun onDestroy() {
        println("GameManager: Cleaning up game...")
        scope.cancel()
    }

    private fun createGameObjects() {
        scope.launch {
            delay(1000) // Wait 1 second after game starts

            // Create some dynamic objects
            for (i in 1..3) {
                createCube(i)
            }
        }
    }

    private fun createCube(index: Int) {
        val entity = world.createEntity("DynamicCube_$index")

        entity.addComponent<TransformComponent> {
            position = Vector3(
                x = (index - 2) * 2.0f,
                y = 5.0f,
                z = 0.0f
            )
            scale = Vector3(0.5f, 0.5f, 0.5f)
        }

        entity.addComponent<MeshRenderer> {
            mesh = "cube.obj"
            material = createMaterial("cube_material_$index")
        }

        entity.addComponent<PhysicsComponent> {
            type = PhysicsType.Dynamic
            mass = 1.0f
            useGravity = true
        }

        entityCount++
        println("GameManager: Created cube $index")
    }

    private fun updateGameState(deltaTime: Float) {
        val currentTime = Time.time - gameStartTime

        // Example: Spawn objects periodically
        if (currentTime > 5.0 && entityCount < 5) {
            createCube(entityCount + 1)
        }

        // Update game statistics
        if (currentTime.toInt() % 10 == 0) {
            println("GameManager: Game running for ${currentTime.toInt()} seconds")
        }
    }

    private fun handleGameEvents() {
        // Handle game-specific events
        // This would typically respond to user input, collisions, etc.
    }

    private fun createMaterial(name: String): Material {
        return Material().apply {
            baseColor = Color(
                r = (Math.random() * 0.8 + 0.2).toFloat(),
                g = (Math.random() * 0.8 + 0.2).toFloat(),
                b = (Math.random() * 0.8 + 0.2).toFloat()
            )
            metallic = 0.0f
            roughness = 0.7f
        }
    }
}

// Extension function for creating materials
fun Material.apply(block: Material.() -> Unit): Material {
    this.block()
    return this
}
