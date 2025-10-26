/**
 * @file AndroidFileSystem.cpp
 * @brief Complete Android file system implementation with native Android APIs
 */

#include "../core/AndroidPlatform.h"
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <unistd.h>
#include <jni.h>

#define LOG_TAG "AndroidFileSystem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace FoundryEngine {

class AndroidFileSystemImpl : public AndroidFileSystem {
private:
    std::string internalStoragePath_;
    std::string externalStoragePath_;
    std::string cachePath_;
    AAssetManager* assetManager_;
    JNIEnv* env_;
    jobject context_;

public:
    AndroidFileSystemImpl(JNIEnv* env, jobject context) : env_(env), context_(context), assetManager_(nullptr) {
        initializePaths();
    }

    void initializePaths() {
        // Get internal storage path
        jclass contextClass = env_->GetObjectClass(context_);
        jmethodID getFilesDirMethod = env_->GetMethodID(contextClass, "getFilesDir", "()Ljava/io/File;");
        jobject filesDir = env_->CallObjectMethod(context_, getFilesDirMethod);
        
        jclass fileClass = env_->GetObjectClass(filesDir);
        jmethodID getAbsolutePathMethod = env_->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
        jstring internalPath = (jstring)env_->CallObjectMethod(filesDir, getAbsolutePathMethod);
        
        const char* internalPathStr = env_->GetStringUTFChars(internalPath, nullptr);
        internalStoragePath_ = std::string(internalPathStr);
        env_->ReleaseStringUTFChars(internalPath, internalPathStr);

        // Get external storage path
        jmethodID getExternalFilesDirMethod = env_->GetMethodID(contextClass, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
        jobject externalDir = env_->CallObjectMethod(context_, getExternalFilesDirMethod, nullptr);
        
        if (externalDir) {
            jstring externalPath = (jstring)env_->CallObjectMethod(externalDir, getAbsolutePathMethod);
            const char* externalPathStr = env_->GetStringUTFChars(externalPath, nullptr);
            externalStoragePath_ = std::string(externalPathStr);
            env_->ReleaseStringUTFChars(externalPath, externalPathStr);
        }

        // Get cache path
        jmethodID getCacheDirMethod = env_->GetMethodID(contextClass, "getCacheDir", "()Ljava/io/File;");
        jobject cacheDir = env_->CallObjectMethod(context_, getCacheDirMethod);
        jstring cachePath = (jstring)env_->CallObjectMethod(cacheDir, getAbsolutePathMethod);
        
        const char* cachePathStr = env_->GetStringUTFChars(cachePath, nullptr);
        cachePath_ = std::string(cachePathStr);
        env_->ReleaseStringUTFChars(cachePath, cachePathStr);

        LOGI("Android paths initialized - Internal: %s, External: %s, Cache: %s", 
             internalStoragePath_.c_str(), externalStoragePath_.c_str(), cachePath_.c_str());
    }

    void setAssetManager(AAssetManager* assetManager) {
        assetManager_ = assetManager;
    }

    std::vector<uint8_t> readFile(const std::string& path) override {
        std::vector<uint8_t> data;
        
        // Try to read from assets first
        if (assetManager_ && path.find("assets/") == 0) {
            std::string assetPath = path.substr(7); // Remove "assets/" prefix
            AAsset* asset = AAssetManager_open(assetManager_, assetPath.c_str(), AASSET_MODE_BUFFER);
            
            if (asset) {
                off_t length = AAsset_getLength(asset);
                data.resize(length);
                AAsset_read(asset, data.data(), length);
                AAsset_close(asset);
                LOGI("Read asset file: %s (%d bytes)", assetPath.c_str(), (int)length);
                return data;
            }
        }

        // Try to read from file system
        std::string fullPath = resolvePath(path);
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            
            data.resize(size);
            if (file.read(reinterpret_cast<char*>(data.data()), size)) {
                LOGI("Read file: %s (%d bytes)", fullPath.c_str(), (int)size);
            } else {
                LOGE("Failed to read file content: %s", fullPath.c_str());
                data.clear();
            }
            file.close();
        } else {
            LOGE("Failed to open file: %s", fullPath.c_str());
        }
        
        return data;
    }

    void writeFile(const std::string& path, const std::vector<uint8_t>& data) override {
        std::string fullPath = resolvePath(path);
        
        // Create directory if it doesn't exist
        size_t lastSlash = fullPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string dir = fullPath.substr(0, lastSlash);
            createDirectoryRecursive(dir);
        }
        
        std::ofstream file(fullPath, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            file.close();
            LOGI("Wrote file: %s (%d bytes)", fullPath.c_str(), (int)data.size());
        } else {
            LOGE("Failed to write file: %s", fullPath.c_str());
        }
    }

    void deleteFile(const std::string& path) override {
        std::string fullPath = resolvePath(path);
        if (unlink(fullPath.c_str()) == 0) {
            LOGI("Deleted file: %s", fullPath.c_str());
        } else {
            LOGE("Failed to delete file: %s", fullPath.c_str());
        }
    }

    std::vector<std::string> listFiles(const std::string& directory) override {
        std::vector<std::string> files;
        std::string fullPath = resolvePath(directory);
        
        DIR* dir = opendir(fullPath.c_str());
        if (dir) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                if (entry->d_name[0] != '.') { // Skip hidden files
                    files.push_back(entry->d_name);
                }
            }
            closedir(dir);
            LOGI("Listed %d files in directory: %s", (int)files.size(), fullPath.c_str());
        } else {
            LOGE("Failed to open directory: %s", fullPath.c_str());
        }
        
        return files;
    }

    void createDirectory(const std::string& path) override {
        std::string fullPath = resolvePath(path);
        createDirectoryRecursive(fullPath);
    }

    bool exists(const std::string& path) override {
        // Check assets first
        if (assetManager_ && path.find("assets/") == 0) {
            std::string assetPath = path.substr(7);
            AAsset* asset = AAssetManager_open(assetManager_, assetPath.c_str(), AASSET_MODE_BUFFER);
            if (asset) {
                AAsset_close(asset);
                return true;
            }
        }
        
        // Check file system
        std::string fullPath = resolvePath(path);
        struct stat buffer;
        return (stat(fullPath.c_str(), &buffer) == 0);
    }

private:
    std::string resolvePath(const std::string& path) {
        if (path.empty()) return internalStoragePath_;
        
        // Absolute path
        if (path[0] == '/') return path;
        
        // Relative to internal storage
        if (path.find("cache/") == 0) {
            return cachePath_ + "/" + path.substr(6);
        }
        
        if (path.find("external/") == 0 && !externalStoragePath_.empty()) {
            return externalStoragePath_ + "/" + path.substr(9);
        }
        
        return internalStoragePath_ + "/" + path;
    }

    void createDirectoryRecursive(const std::string& path) {
        size_t pos = 0;
        std::string dir;
        
        while ((pos = path.find('/', pos)) != std::string::npos) {
            dir = path.substr(0, pos++);
            if (!dir.empty()) {
                mkdir(dir.c_str(), 0755);
            }
        }
        
        mkdir(path.c_str(), 0755);
        LOGI("Created directory: %s", path.c_str());
    }
};

// Global instance
static std::unique_ptr<AndroidFileSystemImpl> g_fileSystem;

} // namespace FoundryEngine

// JNI functions
extern "C" {

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeInitializeFileSystem(JNIEnv* env, jobject thiz, jobject assetManager) {
    using namespace FoundryEngine;
    
    g_fileSystem = std::make_unique<AndroidFileSystemImpl>(env, thiz);
    
    if (assetManager) {
        AAssetManager* nativeAssetManager = AAssetManager_fromJava(env, assetManager);
        g_fileSystem->setAssetManager(nativeAssetManager);
    }
    
    LOGI("Android file system initialized");
}

JNIEXPORT jbyteArray JNICALL Java_com_foundryengine_game_GameActivity_nativeReadFile(JNIEnv* env, jobject thiz, jstring path) {
    if (!g_fileSystem) return nullptr;
    
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    std::vector<uint8_t> data = g_fileSystem->readFile(pathStr);
    env->ReleaseStringUTFChars(path, pathStr);
    
    if (data.empty()) return nullptr;
    
    jbyteArray result = env->NewByteArray(data.size());
    env->SetByteArrayRegion(result, 0, data.size(), reinterpret_cast<jbyte*>(data.data()));
    return result;
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeWriteFile(JNIEnv* env, jobject thiz, jstring path, jbyteArray data) {
    if (!g_fileSystem) return;
    
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    jsize length = env->GetArrayLength(data);
    jbyte* dataPtr = env->GetByteArrayElements(data, nullptr);
    
    std::vector<uint8_t> fileData(reinterpret_cast<uint8_t*>(dataPtr), 
                                  reinterpret_cast<uint8_t*>(dataPtr) + length);
    
    g_fileSystem->writeFile(pathStr, fileData);
    
    env->ReleaseStringUTFChars(path, pathStr);
    env->ReleaseByteArrayElements(data, dataPtr, JNI_ABORT);
}

JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeDeleteFile(JNIEnv* env, jobject thiz, jstring path) {
    if (!g_fileSystem) return;
    
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    g_fileSystem->deleteFile(pathStr);
    env->ReleaseStringUTFChars(path, pathStr);
}

JNIEXPORT jobjectArray JNICALL Java_com_foundryengine_game_GameActivity_nativeListFiles(JNIEnv* env, jobject thiz, jstring directory) {
    if (!g_fileSystem) return nullptr;
    
    const char* dirStr = env->GetStringUTFChars(directory, nullptr);
    std::vector<std::string> files = g_fileSystem->listFiles(dirStr);
    env->ReleaseStringUTFChars(directory, dirStr);
    
    jobjectArray result = env->NewObjectArray(files.size(), env->FindClass("java/lang/String"), nullptr);
    for (size_t i = 0; i < files.size(); ++i) {
        jstring fileName = env->NewStringUTF(files[i].c_str());
        env->SetObjectArrayElement(result, i, fileName);
        env->DeleteLocalRef(fileName);
    }
    
    return result;
}

JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeFileExists(JNIEnv* env, jobject thiz, jstring path) {
    if (!g_fileSystem) return JNI_FALSE;
    
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    bool exists = g_fileSystem->exists(pathStr);
    env->ReleaseStringUTFChars(path, pathStr);
    
    return exists ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"