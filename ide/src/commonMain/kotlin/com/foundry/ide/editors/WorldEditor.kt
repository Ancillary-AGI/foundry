package com.foundry.ide.editors

import com.foundry.ide.*
import kotlinx.coroutines.*
import kotlinx.serialization.Serializable
import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths

/**
 * Visual world editor for Foundry IDE
 * Provides drag-and-drop entity creation and scene editing
 */
@Serializable
data class SceneView(
    val camera: CameraInfo,
    val entities: List<EntityView>,
    val lights: List<LightView>,
    val grid: GridSettings,
    val selection: List<String> = emptyList(),
    val gizmos: List<Gizmo> = emptyList()
)

@Serializable
data class CameraInfo(
    val position: Vector3,
    val rotation: Quaternion,
    val fov: Float,
    val near: Float,
    val far: Float
)

@Serializable
data class EntityView(
    val id: String,
    val name: String,
    val position: Vector3,
    val rotation: Quaternion,
    val scale: Vector3,
    val components: List<ComponentView>,
    val children: List<String> = emptyList(),
    val visible: Boolean = true,
    val locked: Boolean = false
)

@Serializable
data class ComponentView(
    val id: String,
    val type: String,
    val name: String,
    val properties: Map<String, Any> = emptyMap(),
    val enabled: Boolean = true
)

@Serializable
data class LightView(
    val id: String,
    val type: LightType,
    val position: Vector3,
    val rotation: Quaternion,
    val color: Color,
    val intensity: Float,
    val range: Float,
    val shadows: Boolean = true
)

@Serializable
data class GridSettings(
    val enabled: Boolean = true,
    val size: Float = 1.0f,
    val subdivisions: Int = 10,
    val color: Color = Color(0.5f, 0.5f, 0.5f, 0.3f)
)

@Serializable
data class Gizmo(
    val id: String,
    val type: GizmoType,
    val position: Vector3,
    val rotation: Quaternion,
    val scale: Vector3,
    val visible: Boolean = true
)

enum class GizmoType {
    TRANSLATION, ROTATION, SCALE, BOUNDING_BOX
}

@Serializable
data class Vector3(
    val x: Float = 0.0f,
    val y: Float = 0.0f,
    val z: Float = 0.0f
)

@Serializable
data class Quaternion(
    val x: Float = 0.0f,
    val y: Float = 0.0f,
    val z: Float = 0.0f,
    val w: Float = 1.0f
)

@Serializable
data class Color(
    val r: Float,
    val g: Float,
    val b: Float,
    val a: Float = 1.0f
)

enum class LightType {
    DIRECTIONAL, POINT, SPOT, AREA
}

class WorldEditor {
    private val scope = CoroutineScope(Dispatchers.Default + SupervisorJob())
    private val sceneViews = mutableMapOf<String, SceneView>()
    private val entityTemplates = mutableMapOf<String, EntityTemplate>()

    private var activeScene: String? = null
    private var nextEntityId = 0

    init {
        loadEntityTemplates()
    }

    /**
     * Load or create scene view
     */
    fun loadScene(scenePath: String): SceneView? {
        return try {
            val sceneFile = File(scenePath)
            if (!sceneFile.exists()) {
                return createDefaultScene(scenePath)
            }

            // In a real implementation, parse the scene file
            val sceneContent = Files.readString(sceneFile.toPath())

            // For now, create a default scene view
            val sceneView = createDefaultSceneView(scenePath)
            sceneViews[scenePath] = sceneView
            activeScene = scenePath

            sceneView
        } catch (e: Exception) {
            println("Failed to load scene: ${e.message}")
            null
        }
    }

    /**
     * Save scene view
     */
    fun saveScene(scenePath: String): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false

            // In a real implementation, serialize scene view to scene file format
            val sceneContent = serializeSceneView(sceneView)

            Files.writeString(Paths.get(scenePath), sceneContent)
            true
        } catch (e: Exception) {
            println("Failed to save scene: ${e.message}")
            false
        }
    }

    /**
     * Create entity in scene
     */
    fun createEntity(
        scenePath: String,
        templateId: String,
        position: Vector3 = Vector3(),
        name: String? = null
    ): EntityView? {
        return try {
            val sceneView = sceneViews[scenePath] ?: return null
            val template = entityTemplates[templateId] ?: return null

            val entityName = name ?: "${template.name}_${nextEntityId++}"
            val entityId = generateEntityId()

            val entityView = EntityView(
                id = entityId,
                name = entityName,
                position = position,
                rotation = Quaternion(),
                scale = Vector3(1.0f, 1.0f, 1.0f),
                components = template.components.map { it.copy(id = generateComponentId()) }
            )

            val updatedEntities = sceneView.entities + entityView
            val updatedSceneView = sceneView.copy(entities = updatedEntities)

            sceneViews[scenePath] = updatedSceneView

            entityView
        } catch (e: Exception) {
            println("Failed to create entity: ${e.message}")
            null
        }
    }

    /**
     * Delete entity from scene
     */
    fun deleteEntity(scenePath: String, entityId: String): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false

            val updatedEntities = sceneView.entities.filter { it.id != entityId }
            val updatedSceneView = sceneView.copy(entities = updatedEntities)

            sceneViews[scenePath] = updatedSceneView
            true
        } catch (e: Exception) {
            println("Failed to delete entity: ${e.message}")
            false
        }
    }

    /**
     * Select entities
     */
    fun selectEntities(scenePath: String, entityIds: List<String>): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false

            val updatedSceneView = sceneView.copy(selection = entityIds)
            sceneViews[scenePath] = updatedSceneView
            true
        } catch (e: Exception) {
            println("Failed to select entities: ${e.message}")
            false
        }
    }

    /**
     * Transform selected entities
     */
    fun transformEntities(
        scenePath: String,
        transformType: TransformType,
        delta: Vector3
    ): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false
            val selectedIds = sceneView.selection

            if (selectedIds.isEmpty()) return false

            val updatedEntities = sceneView.entities.map { entity ->
                if (entity.id in selectedIds) {
                    when (transformType) {
                        TransformType.TRANSLATE -> entity.copy(position = entity.position + delta)
                        TransformType.ROTATE -> entity.copy(rotation = applyRotation(entity.rotation, delta))
                        TransformType.SCALE -> entity.copy(scale = entity.scale + delta)
                    }
                } else {
                    entity
                }
            }

            val updatedSceneView = sceneView.copy(entities = updatedEntities)
            sceneViews[scenePath] = updatedSceneView
            true
        } catch (e: Exception) {
            println("Failed to transform entities: ${e.message}")
            false
        }
    }

    /**
     * Add component to entity
     */
    fun addComponent(
        scenePath: String,
        entityId: String,
        componentType: String,
        properties: Map<String, Any> = emptyMap()
    ): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false

            val updatedEntities = sceneView.entities.map { entity ->
                if (entity.id == entityId) {
                    val newComponent = ComponentView(
                        id = generateComponentId(),
                        type = componentType,
                        name = componentType,
                        properties = properties,
                        enabled = true
                    )

                    entity.copy(components = entity.components + newComponent)
                } else {
                    entity
                }
            }

            val updatedSceneView = sceneView.copy(entities = updatedEntities)
            sceneViews[scenePath] = updatedSceneView
            true
        } catch (e: Exception) {
            println("Failed to add component: ${e.message}")
            false
        }
    }

    /**
     * Remove component from entity
     */
    fun removeComponent(scenePath: String, entityId: String, componentId: String): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false

            val updatedEntities = sceneView.entities.map { entity ->
                if (entity.id == entityId) {
                    entity.copy(components = entity.components.filter { it.id != componentId })
                } else {
                    entity
                }
            }

            val updatedSceneView = sceneView.copy(entities = updatedEntities)
            sceneViews[scenePath] = updatedSceneView
            true
        } catch (e: Exception) {
            println("Failed to remove component: ${e.message}")
            false
        }
    }

    /**
     * Update component property
     */
    fun updateComponentProperty(
        scenePath: String,
        entityId: String,
        componentId: String,
        propertyName: String,
        value: Any
    ): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false

            val updatedEntities = sceneView.entities.map { entity ->
                if (entity.id == entityId) {
                    val updatedComponents = entity.components.map { component ->
                        if (component.id == componentId) {
                            component.copy(properties = component.properties + (propertyName to value))
                        } else {
                            component
                        }
                    }

                    entity.copy(components = updatedComponents)
                } else {
                    entity
                }
            }

            val updatedSceneView = sceneView.copy(entities = updatedEntities)
            sceneViews[scenePath] = updatedSceneView
            true
        } catch (e: Exception) {
            println("Failed to update component property: ${e.message}")
            false
        }
    }

    /**
     * Get scene view
     */
    fun getSceneView(scenePath: String): SceneView? {
        return sceneViews[scenePath]
    }

    /**
     * Get available entity templates
     */
    fun getEntityTemplates(): List<EntityTemplate> {
        return entityTemplates.values.toList()
    }

    /**
     * Create entity template
     */
    fun createEntityTemplate(
        name: String,
        components: List<ComponentView>
    ): String {
        val templateId = generateTemplateId()
        val template = EntityTemplate(
            id = templateId,
            name = name,
            components = components
        )

        entityTemplates[templateId] = template
        saveEntityTemplates()

        return templateId
    }

    /**
     * Update camera position
     */
    fun updateCamera(scenePath: String, position: Vector3, rotation: Quaternion): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false

            val updatedCamera = sceneView.camera.copy(position = position, rotation = rotation)
            val updatedSceneView = sceneView.copy(camera = updatedCamera)

            sceneViews[scenePath] = updatedSceneView
            true
        } catch (e: Exception) {
            println("Failed to update camera: ${e.message}")
            false
        }
    }

    /**
     * Toggle grid visibility
     */
    fun toggleGrid(scenePath: String): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false

            val updatedGrid = sceneView.grid.copy(enabled = !sceneView.grid.enabled)
            val updatedSceneView = sceneView.copy(grid = updatedGrid)

            sceneViews[scenePath] = updatedSceneView
            true
        } catch (e: Exception) {
            println("Failed to toggle grid: ${e.message}")
            false
        }
    }

    /**
     * Create default scene view
     */
    private fun createDefaultSceneView(scenePath: String): SceneView {
        return SceneView(
            camera = CameraInfo(
                position = Vector3(0.0f, 5.0f, 10.0f),
                rotation = Quaternion(),
                fov = 60.0f,
                near = 0.1f,
                far = 1000.0f
            ),
            entities = listOf(
                EntityView(
                    id = "ground",
                    name = "Ground",
                    position = Vector3(0.0f, 0.0f, 0.0f),
                    rotation = Quaternion(),
                    scale = Vector3(10.0f, 1.0f, 10.0f),
                    components = listOf(
                        ComponentView(
                            id = "ground_mesh",
                            type = "MeshRenderer",
                            name = "Mesh Renderer",
                            properties = mapOf(
                                "mesh" to "plane.obj",
                                "material" to "ground_material"
                            )
                        ),
                        ComponentView(
                            id = "ground_physics",
                            type = "PhysicsComponent",
                            name = "Physics Body",
                            properties = mapOf(
                                "type" to "static",
                                "mass" to 0.0f
                            )
                        )
                    )
                )
            ),
            lights = listOf(
                LightView(
                    id = "directional_light",
                    type = LightType.DIRECTIONAL,
                    position = Vector3(0.0f, 10.0f, 0.0f),
                    rotation = Quaternion(),
                    color = Color(1.0f, 0.9f, 0.8f),
                    intensity = 1.0f,
                    range = 100.0f,
                    shadows = true
                )
            ),
            grid = GridSettings()
        )
    }

    /**
     * Create default scene
     */
    private fun createDefaultScene(scenePath: String): SceneView {
        val sceneView = createDefaultSceneView(scenePath)

        // Save default scene
        try {
            val sceneContent = serializeSceneView(sceneView)
            Files.writeString(Paths.get(scenePath), sceneContent)
        } catch (e: Exception) {
            println("Failed to save default scene: ${e.message}")
        }

        return sceneView
    }

    /**
     * Serialize scene view to scene file format
     */
    private fun serializeSceneView(sceneView: SceneView): String {
        // In a real implementation, this would serialize to the proper scene format
        return """
        {
          "name": "Scene",
          "camera": {
            "position": [${sceneView.camera.position.x}, ${sceneView.camera.position.y}, ${sceneView.camera.position.z}],
            "rotation": [${sceneView.camera.rotation.x}, ${sceneView.camera.rotation.y}, ${sceneView.camera.rotation.z}, ${sceneView.camera.rotation.w}],
            "fov": ${sceneView.camera.fov}
          },
          "entities": [
            ${sceneView.entities.joinToString(",\n            ") { serializeEntity(it) }}
          ],
          "lights": [
            ${sceneView.lights.joinToString(",\n            ") { serializeLight(it) }}
          ]
        }
        """.trimIndent()
    }

    /**
     * Serialize entity to JSON
     */
    private fun serializeEntity(entity: EntityView): String {
        return """
        {
          "id": "${entity.id}",
          "name": "${entity.name}",
          "position": [${entity.position.x}, ${entity.position.y}, ${entity.position.z}],
          "rotation": [${entity.rotation.x}, ${entity.rotation.y}, ${entity.rotation.z}, ${entity.rotation.w}],
          "scale": [${entity.scale.x}, ${entity.scale.y}, ${entity.scale.z}],
          "components": [
            ${entity.components.joinToString(",\n            ") { serializeComponent(it) }}
          ]
        }
        """.trimIndent()
    }

    /**
     * Serialize component to JSON
     */
    private fun serializeComponent(component: ComponentView): String {
        return """
        {
          "id": "${component.id}",
          "type": "${component.type}",
          "name": "${component.name}",
          "enabled": ${component.enabled}
        }
        """.trimIndent()
    }

    /**
     * Serialize light to JSON
     */
    private fun serializeLight(light: LightView): String {
        return """
        {
          "id": "${light.id}",
          "type": "${light.type}",
          "position": [${light.position.x}, ${light.position.y}, ${light.position.z}],
          "color": [${light.color.r}, ${light.color.g}, ${light.color.b}],
          "intensity": ${light.intensity}
        }
        """.trimIndent()
    }

    /**
     * Apply rotation to quaternion
     */
    private fun applyRotation(quaternion: Quaternion, delta: Vector3): Quaternion {
        // Simple rotation application (in a real implementation, use proper quaternion math)
        return Quaternion(
            x = quaternion.x + delta.x * 0.1f,
            y = quaternion.y + delta.y * 0.1f,
            z = quaternion.z + delta.z * 0.1f,
            w = quaternion.w
        )
    }

    /**
     * Load entity templates
     */
    private fun loadEntityTemplates() {
        // Basic entity templates
        entityTemplates["cube"] = EntityTemplate(
            id = "cube",
            name = "Cube",
            components = listOf(
                ComponentView(
                    id = "mesh",
                    type = "MeshRenderer",
                    name = "Mesh Renderer",
                    properties = mapOf("mesh" to "cube.obj")
                ),
                ComponentView(
                    id = "physics",
                    type = "PhysicsComponent",
                    name = "Physics Body",
                    properties = mapOf("mass" to 1.0f)
                )
            )
        )

        entityTemplates["sphere"] = EntityTemplate(
            id = "sphere",
            name = "Sphere",
            components = listOf(
                ComponentView(
                    id = "mesh",
                    type = "MeshRenderer",
                    name = "Mesh Renderer",
                    properties = mapOf("mesh" to "sphere.obj")
                ),
                ComponentView(
                    id = "physics",
                    type = "PhysicsComponent",
                    name = "Physics Body",
                    properties = mapOf("mass" to 1.0f)
                )
            )
        )

        entityTemplates["light"] = EntityTemplate(
            id = "light",
            name = "Light",
            components = listOf(
                ComponentView(
                    id = "light",
                    type = "LightComponent",
                    name = "Light",
                    properties = mapOf(
                        "type" to "point",
                        "intensity" to 1.0f,
                        "range" to 10.0f
                    )
                )
            )
        )

        entityTemplates["camera"] = EntityTemplate(
            id = "camera",
            name = "Camera",
            components = listOf(
                ComponentView(
                    id = "camera",
                    type = "CameraComponent",
                    name = "Camera",
                    properties = mapOf(
                        "fov" to 60.0f,
                        "near" to 0.1f,
                        "far" to 1000.0f
                    )
                )
            )
        )
    }

    /**
     * Save entity templates
     */
    private fun saveEntityTemplates() {
        try {
            val templatesDir = File("ide/templates/entities")
            templatesDir.mkdirs()

            entityTemplates.values.forEach { template ->
                val templateFile = templatesDir.resolve("${template.id}.json")
                // In a real implementation, serialize template to JSON
            }
        } catch (e: Exception) {
            println("Failed to save entity templates: ${e.message}")
        }
    }

    /**
     * Generate unique entity ID
     */
    private fun generateEntityId(): String {
        return "entity_${System.currentTimeMillis()}_${nextEntityId++}"
    }

    /**
     * Generate unique component ID
     */
    private fun generateComponentId(): String {
        return "component_${System.currentTimeMillis()}_${kotlin.random.Random.nextInt(1000)}"
    }

    /**
     * Generate unique template ID
     */
    private fun generateTemplateId(): String {
        return "template_${System.currentTimeMillis()}"
    }

    /**
     * Get active scene
     */
    fun getActiveScene(): String? {
        return activeScene
    }

    /**
     * Set active scene
     */
    fun setActiveScene(scenePath: String) {
        activeScene = scenePath
    }

    /**
     * Duplicate entity
     */
    fun duplicateEntity(scenePath: String, entityId: String): String? {
        return try {
            val sceneView = sceneViews[scenePath] ?: return null
            val originalEntity = sceneView.entities.find { it.id == entityId } ?: return null

            val duplicatedEntity = originalEntity.copy(
                id = generateEntityId(),
                name = "${originalEntity.name}_Copy",
                position = originalEntity.position + Vector3(2.0f, 0.0f, 0.0f)
            )

            val updatedEntities = sceneView.entities + duplicatedEntity
            val updatedSceneView = sceneView.copy(entities = updatedEntities)

            sceneViews[scenePath] = updatedSceneView

            duplicatedEntity.id
        } catch (e: Exception) {
            println("Failed to duplicate entity: ${e.message}")
            null
        }
    }

    /**
     * Get entity hierarchy
     */
    fun getEntityHierarchy(scenePath: String): List<EntityHierarchyNode> {
        val sceneView = sceneViews[scenePath] ?: return emptyList()

        return sceneView.entities.map { entity ->
            EntityHierarchyNode(
                id = entity.id,
                name = entity.name,
                type = "Entity",
                children = getEntityChildren(entity, sceneView.entities),
                visible = entity.visible,
                selected = entity.id in sceneView.selection
            )
        }
    }

    /**
     * Get entity children
     */
    private fun getEntityChildren(entity: EntityView, allEntities: List<EntityView>): List<EntityHierarchyNode> {
        return entity.children.mapNotNull { childId ->
            allEntities.find { it.id == childId }?.let { childEntity ->
                EntityHierarchyNode(
                    id = childEntity.id,
                    name = childEntity.name,
                    type = "Entity",
                    children = getEntityChildren(childEntity, allEntities),
                    visible = childEntity.visible,
                    selected = false
                )
            }
        }
    }

    /**
     * Focus camera on entity
     */
    fun focusCameraOnEntity(scenePath: String, entityId: String): Boolean {
        return try {
            val sceneView = sceneViews[scenePath] ?: return false
            val entity = sceneView.entities.find { it.id == entityId } ?: return false

            // Calculate camera position to focus on entity
            val focusPosition = entity.position + Vector3(0.0f, 2.0f, 5.0f)
            val focusRotation = calculateLookAtRotation(focusPosition, entity.position)

            updateCamera(scenePath, focusPosition, focusRotation)
        } catch (e: Exception) {
            println("Failed to focus camera: ${e.message}")
            false
        }
    }

    /**
     * Calculate look-at rotation
     */
    private fun calculateLookAtRotation(cameraPosition: Vector3, targetPosition: Vector3): Quaternion {
        // Simple look-at calculation (in a real implementation, use proper quaternion math)
        val direction = targetPosition - cameraPosition
        return Quaternion(direction.x * 0.1f, direction.y * 0.1f, direction.z * 0.1f, 1.0f)
    }

    /**
     * Shutdown world editor
     */
    fun shutdown() {
        // Save all scenes
        sceneViews.keys.forEach { scenePath ->
            saveScene(scenePath)
        }

        sceneViews.clear()
        activeScene = null
    }
}

/**
 * Entity template data class
 */
@Serializable
data class EntityTemplate(
    val id: String,
    val name: String,
    val description: String = "",
    val icon: String = "",
    val category: String = "General",
    val components: List<ComponentView>
)

/**
 * Entity hierarchy node
 */
@Serializable
data class EntityHierarchyNode(
    val id: String,
    val name: String,
    val type: String,
    val children: List<EntityHierarchyNode> = emptyList(),
    val visible: Boolean = true,
    val selected: Boolean = false
)

/**
 * Transform types
 */
enum class TransformType {
    TRANSLATE, ROTATE, SCALE
}

/**
 * Vector3 operator overloads
 */
operator fun Vector3.plus(other: Vector3): Vector3 {
    return Vector3(x + other.x, y + other.y, z + other.z)
}

operator fun Vector3.minus(other: Vector3): Vector3 {
    return Vector3(x - other.x, y - other.y, z - other.z)
}
