#ifndef FOUNDRY_ENGINE_JNI_H
#define FOUNDRY_ENGINE_JNI_H

#include <jni.h>
#include <string>
#include <vector>
#include "EngineIntegration.h"

/**
 * JNI Bridge for Foundry Engine Integration
 * Provides native methods for Java/Kotlin to communicate with C++ engine
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * JNI Initialization
 */
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved);

/**
 * Engine Integration Methods
 */
JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeInitialize(
    JNIEnv* env, jobject obj, jstring configJson);

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeCreateProject(
    JNIEnv* env, jobject obj, jstring projectJson);

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeLoadProject(
    JNIEnv* env, jobject obj, jstring path);

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeSaveProject(
    JNIEnv* env, jobject obj, jstring projectJson);

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeBuildProject(
    JNIEnv* env, jobject obj, jstring target);

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeRunProject(
    JNIEnv* env, jobject obj, jstring target);

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeStopProject(
    JNIEnv* env, jobject obj);

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeGetProjectInfo(
    JNIEnv* env, jobject obj);

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeCreateEntity(
    JNIEnv* env, jobject obj, jstring name, jstring componentsJson);

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeRemoveEntity(
    JNIEnv* env, jobject obj, jstring entityId);

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeAddComponent(
    JNIEnv* env, jobject obj, jstring entityId, jstring componentType);

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeRemoveComponent(
    JNIEnv* env, jobject obj, jstring entityId, jstring componentId);

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeUpdateEntityTransform(
    JNIEnv* env, jobject obj, jstring entityId, jstring transformJson);

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeGetAvailableComponents(
    JNIEnv* env, jobject obj);

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeGetAvailableSystems(
    JNIEnv* env, jobject obj);

JNIEXPORT void JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeDispose(
    JNIEnv* env, jobject obj);

/**
 * Utility Functions
 */
std::string jstringToStdString(JNIEnv* env, jstring jstr);
jstring stdStringToJstring(JNIEnv* env, const std::string& str);
std::vector<std::string> jstringArrayToVector(JNIEnv* env, jobjectArray array);

/**
 * JNI Bridge Class
 */
class FoundryEngineJNI {
private:
    static FoundryEngineJNI* instance;
    JavaVM* jvm;
    EngineIntegration* engine;

    FoundryEngineJNI();
    ~FoundryEngineJNI();

public:
    static FoundryEngineJNI* getInstance();
    static void cleanup();

    bool initialize(const std::string& configJson);
    bool createProject(const std::string& projectJson);
    std::string loadProject(const std::string& path);
    bool saveProject(const std::string& projectJson);
    std::string buildProject(const std::string& target);
    bool runProject(const std::string& target);
    bool stopProject();
    std::string getProjectInfo();
    std::string createEntity(const std::string& name, const std::vector<std::string>& components);
    bool removeEntity(const std::string& entityId);
    bool addComponent(const std::string& entityId, const std::string& componentType);
    bool removeComponent(const std::string& entityId, const std::string& componentId);
    bool updateEntityTransform(const std::string& entityId, const std::string& transformJson);
    std::string getAvailableComponents();
    std::string getAvailableSystems();
    void dispose();
};

#ifdef __cplusplus
}
#endif

#endif // FOUNDRY_ENGINE_JNI_H
