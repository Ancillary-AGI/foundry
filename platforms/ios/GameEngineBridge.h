//
//  GameEngineBridge.h
//  GameEngine iOS Platform
//
//  C interface for Go engine core
//

#ifndef GameEngineBridge_h
#define GameEngineBridge_h

#include <stdint.h>
#include <stdbool.h>

// Platform capabilities
typedef struct {
    bool hasMetal;
    bool hasOpenGL;
    bool hasVulkan;
    int maxTextureSize;
    const char* renderer;
    const char* vendor;
    const char* version;
} PlatformCapabilities;

// Input types
typedef struct {
    float x, y;
    int id;
} TouchPoint;

typedef struct {
    bool pressed;
    float value;
} ButtonState;

typedef struct {
    bool connected;
    const char* id;
    ButtonState* buttons;
    int buttonCount;
    float* axes;
    int axisCount;
} GamepadState;

typedef struct {
    float x, y;
    bool* buttons;
    int buttonCount;
} MouseState;

// Core engine functions
#ifdef __cplusplus
extern "C" {
#endif

// Engine lifecycle
void* GameEngineCreate(void);
void GameEngineDestroy(void* engine);
bool GameEngineInitialize(void* engine, PlatformCapabilities caps);
void GameEngineStart(void* engine);
void GameEngineStop(void* engine);
void GameEngineUpdate(void* engine, double deltaTime);
void GameEngineRender(void* engine);

// Entity management
uint32_t GameEngineCreateEntity(void* engine, const char* name);
void GameEngineDestroyEntity(void* engine, uint32_t entityId);
uint32_t* GameEngineGetEntities(void* engine, int* count);

// Component management
void GameEngineAddTransformComponent(void* engine, uint32_t entityId, float x, float y, float z);
void GameEngineAddRenderComponent(void* engine, uint32_t entityId, const char* meshData, int meshSize, const char* materialData, int materialSize);
void GameEngineAddNetworkComponent(void* engine, uint32_t entityId);

// Input handling
void GameEngineSetKeyboardState(void* engine, int keyCode, bool pressed);
void GameEngineSetMouseState(void* engine, float x, float y, bool* buttons, int buttonCount);
void GameEngineSetTouchState(void* engine, TouchPoint* touches, int touchCount);
void GameEngineSetGamepadState(void* engine, int index, GamepadState state);

// Networking
void GameEngineStartServer(void* engine, const char* address, int port, int maxClients);
void GameEngineStartClient(void* engine, const char* address, int port);
void GameEngineStopNetworking(void* engine);
bool GameEngineIsNetworkConnected(void* engine);

// Scene management
uint32_t GameEngineCreateScene(void* engine, const char* name);
void GameEngineSetCurrentScene(void* engine, uint32_t sceneId);
void GameEngineAddEntityToScene(void* engine, uint32_t sceneId, uint32_t entityId);

// System management
void GameEngineRegisterSystem(void* engine, const char* systemName, void* systemImpl);
void GameEngineUnregisterSystem(void* engine, const char* systemName);

// Resource management
uint32_t GameEngineLoadTexture(void* engine, const char* path);
uint32_t GameEngineLoadMesh(void* engine, const char* path);
uint32_t GameEngineLoadShader(void* engine, const char* vertexPath, const char* fragmentPath);

// Audio
void GameEnginePlaySound(void* engine, uint32_t soundId, float volume, bool loop);
void GameEngineStopSound(void* engine, uint32_t soundId);

// Physics
void GameEngineSetGravity(void* engine, float x, float y, float z);
uint32_t GameEngineCreateRigidBody(void* engine, uint32_t entityId, float mass);
void GameEngineApplyForce(void* engine, uint32_t bodyId, float x, float y, float z);

// AI
uint32_t GameEngineCreatePath(void* engine, float startX, float startY, float endX, float endY);
void GameEngineUpdateAI(void* engine, double deltaTime);

// Performance monitoring
double GameEngineGetFPS(void* engine);
double GameEngineGetFrameTime(void* engine);
void GameEngineBeginProfile(void* engine, const char* name);
void GameEngineEndProfile(void* engine, const char* name);

// Error handling
const char* GameEngineGetLastError(void* engine);
void GameEngineClearError(void* engine);

#ifdef __cplusplus
}
#endif

#endif /* GameEngineBridge_h */
