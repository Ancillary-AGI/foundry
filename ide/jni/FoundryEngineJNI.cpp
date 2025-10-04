#include "FoundryEngineJNI.h"
#include <iostream>
#include <sstream>
#include "../include/GameEngine/core/EngineIntegration.h"

// Singleton instance
FoundryEngineJNI* FoundryEngineJNI::instance = nullptr;

/**
 * JNI Bridge Implementation
 */

// JNI Initialization
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8) != JNI_OK) {
        return JNI_ERR;
    }

    // Initialize JNI bridge
    FoundryEngineJNI::getInstance()->jvm = vm;

    return JNI_VERSION_1_8;
}

// JNI Method Implementations
JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeInitialize(
    JNIEnv* env, jobject obj, jstring configJson) {

    try {
        std::string configStr = jstringToStdString(env, configJson);
        bool result = FoundryEngineJNI::getInstance()->initialize(configStr);
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeInitialize: " << e.what() << std::endl;
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeCreateProject(
    JNIEnv* env, jobject obj, jstring projectJson) {

    try {
        std::string projectStr = jstringToStdString(env, projectJson);
        bool result = FoundryEngineJNI::getInstance()->createProject(projectStr);
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeCreateProject: " << e.what() << std::endl;
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeLoadProject(
    JNIEnv* env, jobject obj, jstring path) {

    try {
        std::string pathStr = jstringToStdString(env, path);
        std::string result = FoundryEngineJNI::getInstance()->loadProject(pathStr);
        return stdStringToJstring(env, result);
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeLoadProject: " << e.what() << std::endl;
        return stdStringToJstring(env, "null");
    }
}

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeSaveProject(
    JNIEnv* env, jobject obj, jstring projectJson) {

    try {
        std::string projectStr = jstringToStdString(env, projectJson);
        bool result = FoundryEngineJNI::getInstance()->saveProject(projectStr);
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeSaveProject: " << e.what() << std::endl;
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeBuildProject(
    JNIEnv* env, jobject obj, jstring target) {

    try {
        std::string targetStr = jstringToStdString(env, target);
        std::string result = FoundryEngineJNI::getInstance()->buildProject(targetStr);
        return stdStringToJstring(env, result);
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeBuildProject: " << e.what() << std::endl;
        return stdStringToJstring(env, "{\"success\":false,\"errors\":[\"JNI Error: " + std::string(e.what()) + "\"]}");
    }
}

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeRunProject(
    JNIEnv* env, jobject obj, jstring target) {

    try {
        std::string targetStr = jstringToStdString(env, target);
        bool result = FoundryEngineJNI::getInstance()->runProject(targetStr);
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeRunProject: " << e.what() << std::endl;
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeStopProject(
    JNIEnv* env, jobject obj) {

    try {
        bool result = FoundryEngineJNI::getInstance()->stopProject();
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeStopProject: " << e.what() << std::endl;
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeGetProjectInfo(
    JNIEnv* env, jobject obj) {

    try {
        std::string result = FoundryEngineJNI::getInstance()->getProjectInfo();
        return stdStringToJstring(env, result);
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeGetProjectInfo: " << e.what() << std::endl;
        return stdStringToJstring(env, "null");
    }
}

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeCreateEntity(
    JNIEnv* env, jobject obj, jstring name, jstring componentsJson) {

    try {
        std::string nameStr = jstringToStdString(env, name);
        std::string componentsStr = jstringToStdString(env, componentsJson);

        // Parse components JSON to vector
        std::vector<std::string> components;
        std::stringstream ss(componentsStr);
        std::string component;
        while (std::getline(ss, component, ',')) {
            // Trim whitespace and quotes
            component.erase(component.begin(), std::find_if(component.begin(), component.end(), [](int ch) {
                return !std::isspace(ch);
            }));
            component.erase(std::find_if(component.rbegin(), component.rend(), [](int ch) {
                return !std::isspace(ch);
            }).base(), component.end());
            if (component.front() == '"') component.erase(0, 1);
            if (component.back() == '"') component.pop_back();
            components.push_back(component);
        }

        std::string result = FoundryEngineJNI::getInstance()->createEntity(nameStr, components);
        return stdStringToJstring(env, result);
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeCreateEntity: " << e.what() << std::endl;
        return stdStringToJstring(env, "null");
    }
}

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeRemoveEntity(
    JNIEnv* env, jobject obj, jstring entityId) {

    try {
        std::string entityIdStr = jstringToStdString(env, entityId);
        bool result = FoundryEngineJNI::getInstance()->removeEntity(entityIdStr);
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeRemoveEntity: " << e.what() << std::endl;
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeAddComponent(
    JNIEnv* env, jobject obj, jstring entityId, jstring componentType) {

    try {
        std::string entityIdStr = jstringToStdString(env, entityId);
        std::string componentTypeStr = jstringToStdString(env, componentType);
        bool result = FoundryEngineJNI::getInstance()->addComponent(entityIdStr, componentTypeStr);
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeAddComponent: " << e.what() << std::endl;
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeRemoveComponent(
    JNIEnv* env, jobject obj, jstring entityId, jstring componentId) {

    try {
        std::string entityIdStr = jstringToStdString(env, entityId);
        std::string componentIdStr = jstringToStdString(env, componentId);
        bool result = FoundryEngineJNI::getInstance()->removeComponent(entityIdStr, componentIdStr);
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeRemoveComponent: " << e.what() << std::endl;
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeUpdateEntityTransform(
    JNIEnv* env, jobject obj, jstring entityId, jstring transformJson) {

    try {
        std::string entityIdStr = jstringToStdString(env, entityId);
        std::string transformStr = jstringToStdString(env, transformJson);
        bool result = FoundryEngineJNI::getInstance()->updateEntityTransform(entityIdStr, transformStr);
        return result ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeUpdateEntityTransform: " << e.what() << std::endl;
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeGetAvailableComponents(
    JNIEnv* env, jobject obj) {

    try {
        std::string result = FoundryEngineJNI::getInstance()->getAvailableComponents();
        return stdStringToJstring(env, result);
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeGetAvailableComponents: " << e.what() << std::endl;
        return stdStringToJstring(env, "[]");
    }
}

JNIEXPORT jstring JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeGetAvailableSystems(
    JNIEnv* env, jobject obj) {

    try {
        std::string result = FoundryEngineJNI::getInstance()->getAvailableSystems();
        return stdStringToJstring(env, result);
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeGetAvailableSystems: " << e.what() << std::endl;
        return stdStringToJstring(env, "[]");
    }
}

JNIEXPORT void JNICALL Java_com_foundry_ide_JvmEngineIntegration_nativeDispose(
    JNIEnv* env, jobject obj) {

    try {
        FoundryEngineJNI::getInstance()->dispose();
    } catch (const std::exception& e) {
        std::cerr << "JNI Error in nativeDispose: " << e.what() << std::endl;
    }
}

// Utility Functions Implementation
std::string jstringToStdString(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";

    const char* chars = env->GetStringUTFChars(jstr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jstr, chars);
    return result;
}

jstring stdStringToJstring(JNIEnv* env, const std::string& str) {
    return env->NewStringUTF(str.c_str());
}

std::vector<std::string> jstringArrayToVector(JNIEnv* env, jobjectArray array) {
    std::vector<std::string> result;
    if (!array) return result;

    jsize length = env->GetArrayLength(array);
    for (jsize i = 0; i < length; ++i) {
        jstring element = (jstring) env->GetObjectArrayElement(array, i);
        result.push_back(jstringToStdString(env, element));
        env->DeleteLocalRef(element);
    }
    return result;
}

// FoundryEngineJNI Class Implementation
FoundryEngineJNI::FoundryEngineJNI() : jvm(nullptr), engine(nullptr) {
    // Initialize the actual Foundry engine integration
    // This would connect to the real C++ engine
}

FoundryEngineJNI::~FoundryEngineJNI() {
    dispose();
}

FoundryEngineJNI* FoundryEngineJNI::getInstance() {
    if (instance == nullptr) {
        instance = new FoundryEngineJNI();
    }
    return instance;
}

void FoundryEngineJNI::cleanup() {
    if (instance != nullptr) {
        delete instance;
        instance = nullptr;
    }
}

bool FoundryEngineJNI::initialize(const std::string& configJson) {
    try {
        // Initialize the Foundry engine with the provided config
        // This would call the actual C++ engine initialization
        std::cout << "Initializing Foundry Engine with config: " << configJson << std::endl;

        // For now, just return success
        // In a real implementation, this would initialize the actual engine
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize engine: " << e.what() << std::endl;
        return false;
    }
}

bool FoundryEngineJNI::createProject(const std::string& projectJson) {
    try {
        std::cout << "Creating project: " << projectJson << std::endl;
        // In a real implementation, this would create a project in the C++ engine
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create project: " << e.what() << std::endl;
        return false;
    }
}

std::string FoundryEngineJNI::loadProject(const std::string& path) {
    try {
        std::cout << "Loading project from: " << path << std::endl;
        // In a real implementation, this would load a project from the C++ engine
        return "{\"name\":\"Sample Project\",\"path\":\"" + path + "\"}";
    } catch (const std::exception& e) {
        std::cerr << "Failed to load project: " << e.what() << std::endl;
        return "null";
    }
}

bool FoundryEngineJNI::saveProject(const std::string& projectJson) {
    try {
        std::cout << "Saving project: " << projectJson << std::endl;
        // In a real implementation, this would save a project in the C++ engine
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to save project: " << e.what() << std::endl;
        return false;
    }
}

std::string FoundryEngineJNI::buildProject(const std::string& target) {
    try {
        std::cout << "Building project for target: " << target << std::endl;
        // In a real implementation, this would build the project using the C++ engine
        return "{\"success\":true,\"outputPath\":\"./build/" + target + "\"}";
    } catch (const std::exception& e) {
        std::cerr << "Failed to build project: " << e.what() << std::endl;
        return "{\"success\":false,\"errors\":[\"Build failed: " + std::string(e.what()) + "\"]}";
    }
}

bool FoundryEngineJNI::runProject(const std::string& target) {
    try {
        std::cout << "Running project on target: " << target << std::endl;
        // In a real implementation, this would run the project using the C++ engine
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to run project: " << e.what() << std::endl;
        return false;
    }
}

bool FoundryEngineJNI::stopProject() {
    try {
        std::cout << "Stopping project" << std::endl;
        // In a real implementation, this would stop the running project
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to stop project: " << e.what() << std::endl;
        return false;
    }
}

std::string FoundryEngineJNI::getProjectInfo() {
    try {
        std::cout << "Getting project info" << std::endl;
        // In a real implementation, this would get project info from the C++ engine
        return "{\"name\":\"Current Project\",\"entities\":[],\"components\":[],\"systems\":[]}";
    } catch (const std::exception& e) {
        std::cerr << "Failed to get project info: " << e.what() << std::endl;
        return "null";
    }
}

std::string FoundryEngineJNI::createEntity(const std::string& name, const std::vector<std::string>& components) {
    try {
        std::cout << "Creating entity: " << name << " with components: ";
        for (const auto& component : components) {
            std::cout << component << " ";
        }
        std::cout << std::endl;

        // In a real implementation, this would create an entity in the C++ engine
        return "entity_001";
    } catch (const std::exception& e) {
        std::cerr << "Failed to create entity: " << e.what() << std::endl;
        return "null";
    }
}

bool FoundryEngineJNI::removeEntity(const std::string& entityId) {
    try {
        std::cout << "Removing entity: " << entityId << std::endl;
        // In a real implementation, this would remove an entity from the C++ engine
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to remove entity: " << e.what() << std::endl;
        return false;
    }
}

bool FoundryEngineJNI::addComponent(const std::string& entityId, const std::string& componentType) {
    try {
        std::cout << "Adding component " << componentType << " to entity " << entityId << std::endl;
        // In a real implementation, this would add a component to an entity in the C++ engine
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to add component: " << e.what() << std::endl;
        return false;
    }
}

bool FoundryEngineJNI::removeComponent(const std::string& entityId, const std::string& componentId) {
    try {
        std::cout << "Removing component " << componentId << " from entity " << entityId << std::endl;
        // In a real implementation, this would remove a component from an entity in the C++ engine
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to remove component: " << e.what() << std::endl;
        return false;
    }
}

bool FoundryEngineJNI::updateEntityTransform(const std::string& entityId, const std::string& transformJson) {
    try {
        std::cout << "Updating transform for entity " << entityId << ": " << transformJson << std::endl;
        // In a real implementation, this would update an entity's transform in the C++ engine
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to update entity transform: " << e.what() << std::endl;
        return false;
    }
}

std::string FoundryEngineJNI::getAvailableComponents() {
    try {
        std::cout << "Getting available components" << std::endl;
        // In a real implementation, this would get available components from the C++ engine
        return "[{\"id\":\"transform\",\"name\":\"Transform\",\"type\":\"TransformComponent\"},{\"id\":\"mesh\",\"name\":\"Mesh Renderer\",\"type\":\"MeshRenderer\"}]";
    } catch (const std::exception& e) {
        std::cerr << "Failed to get available components: " << e.what() << std::endl;
        return "[]";
    }
}

std::string FoundryEngineJNI::getAvailableSystems() {
    try {
        std::cout << "Getting available systems" << std::endl;
        // In a real implementation, this would get available systems from the C++ engine
        return "[{\"id\":\"physics\",\"name\":\"Physics System\",\"type\":\"PhysicsSystem\"},{\"id\":\"rendering\",\"name\":\"Rendering System\",\"type\":\"RenderingSystem\"}]";
    } catch (const std::exception& e) {
        std::cerr << "Failed to get available systems: " << e.what() << std::endl;
        return "[]";
    }
}

void FoundryEngineJNI::dispose() {
    try {
        std::cout << "Disposing JNI bridge" << std::endl;
        // In a real implementation, this would clean up the C++ engine
    } catch (const std::exception& e) {
        std::cerr << "Error during dispose: " << e.what() << std::endl;
    }
}
