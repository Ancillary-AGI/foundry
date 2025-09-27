#ifndef FOUNDRYENGINE_SCOPED_STORAGE_MANAGER_H
#define FOUNDRYENGINE_SCOPED_STORAGE_MANAGER_H

#include "../../core/System.h"
#include <jni.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

namespace FoundryEngine {

// Forward declarations
class ScopedStorageManager;
class StorageVolume;
class MediaStoreManager;
class SAFManager;

// Storage access modes
enum class StorageAccessMode {
    READ_ONLY,
    WRITE_ONLY,
    READ_WRITE
};

// Storage scopes
enum class StorageScope {
    APP_SPECIFIC,        // App's private directory
    SHARED,              // Shared storage (requires permissions)
    EXTERNAL_PUBLIC,     // Public external storage
    MEDIA_STORE,         // Media store (images, videos, audio)
    DOWNLOADS,           // Downloads directory
    DOCUMENTS,           // Documents directory
    CACHE                // Cache directory
};

// File types for MediaStore
enum class MediaFileType {
    IMAGE,
    VIDEO,
    AUDIO,
    DOCUMENT,
    DOWNLOAD
};

// Storage permissions
enum class StoragePermission {
    READ_EXTERNAL_STORAGE,
    WRITE_EXTERNAL_STORAGE,
    ACCESS_MEDIA_LOCATION,
    READ_MEDIA_IMAGES,
    READ_MEDIA_VIDEO,
    READ_MEDIA_AUDIO,
    READ_MEDIA_VISUAL_USER_SELECTED,
    ACCESS_ALL_FILES  // Android 11+ all files access
};

// Storage volume types
enum class StorageVolumeType {
    INTERNAL,
    EXTERNAL,
    REMOVABLE,
    EMULATED
};

// File operation results
enum class FileOperationResult {
    SUCCESS,
    PERMISSION_DENIED,
    FILE_NOT_FOUND,
    FILE_EXISTS,
    INSUFFICIENT_SPACE,
    IO_ERROR,
    INVALID_ARGUMENT,
    SECURITY_EXCEPTION,
    UNKNOWN_ERROR
};

// File information
struct FileInfo {
    std::string name;
    std::string path;
    std::string absolutePath;
    size_t size;
    time_t lastModified;
    time_t lastAccessed;
    bool isDirectory;
    bool isFile;
    bool isHidden;
    bool isReadable;
    bool isWritable;
    bool isExecutable;
    std::string mimeType;
    std::string extension;
};

// Storage volume information
struct StorageVolumeInfo {
    std::string uuid;
    std::string label;
    StorageVolumeType type;
    std::string rootPath;
    int64_t totalSpace;
    int64_t availableSpace;
    int64_t usedSpace;
    bool isPrimary;
    bool isRemovable;
    bool isEmulated;
    std::string state;
};

// Media file metadata
struct MediaFileMetadata {
    std::string displayName;
    std::string title;
    std::string artist;
    std::string album;
    int duration;
    int width;
    int height;
    double latitude;
    double longitude;
    time_t dateTaken;
    time_t dateAdded;
    time_t dateModified;
    std::string mimeType;
    size_t size;
    std::string bucketDisplayName;
    std::string relativePath;
};

// Callback types
using StorageCallback = std::function<void(FileOperationResult, const std::string&)>;
using FileListCallback = std::function<void(FileOperationResult, const std::vector<FileInfo>&)>;
using PermissionCallback = std::function<void(bool, StoragePermission)>;
using VolumeCallback = std::function<void(const std::vector<StorageVolumeInfo>&)>;

// ========== SCOPED STORAGE MANAGER ==========
class ScopedStorageManager : public System {
private:
    static ScopedStorageManager* instance_;

    MediaStoreManager* mediaStoreManager_;
    SAFManager* safManager_;

    // JNI environment
    JNIEnv* env_;
    jobject context_;

    // Storage permissions
    std::unordered_map<StoragePermission, bool> permissions_;
    std::mutex permissionsMutex_;

    // Storage volumes
    std::vector<StorageVolumeInfo> storageVolumes_;
    std::mutex volumesMutex_;

    // File operations
    std::atomic<bool> initialized_;
    std::thread permissionThread_;
    std::atomic<bool> permissionThreadRunning_;

    // Callbacks
    std::unordered_map<std::string, StorageCallback> storageCallbacks_;
    std::unordered_map<std::string, FileListCallback> fileListCallbacks_;
    std::unordered_map<std::string, PermissionCallback> permissionCallbacks_;
    std::unordered_map<std::string, VolumeCallback> volumeCallbacks_;

public:
    ScopedStorageManager();
    ~ScopedStorageManager();

    static ScopedStorageManager* getInstance();

    // System interface
    void initialize() override;
    void update(float dt) override;
    void shutdown() override;

    // JNI setup
    void setJNIEnvironment(JNIEnv* env, jobject context);

    // Permission management
    void requestPermission(StoragePermission permission, PermissionCallback callback);
    void requestAllPermissions();
    bool hasPermission(StoragePermission permission) const;
    bool hasAllPermissions() const;
    void revokePermission(StoragePermission permission);

    // Storage volume management
    void enumerateVolumes(VolumeCallback callback);
    const std::vector<StorageVolumeInfo>& getVolumes() const;
    StorageVolumeInfo getPrimaryVolume() const;
    StorageVolumeInfo getVolumeByUuid(const std::string& uuid) const;
    int64_t getTotalSpace(const std::string& path) const;
    int64_t getAvailableSpace(const std::string& path) const;
    int64_t getUsedSpace(const std::string& path) const;

    // File operations
    FileOperationResult createFile(const std::string& path, const std::string& fileName);
    FileOperationResult createDirectory(const std::string& path, const std::string& dirName);
    FileOperationResult deleteFile(const std::string& path);
    FileOperationResult deleteDirectory(const std::string& path, bool recursive = false);
    FileOperationResult copyFile(const std::string& sourcePath, const std::string& destPath);
    FileOperationResult moveFile(const std::string& sourcePath, const std::string& destPath);
    FileOperationResult renameFile(const std::string& oldPath, const std::string& newPath);

    // File I/O operations
    std::vector<uint8_t> readFile(const std::string& path);
    FileOperationResult writeFile(const std::string& path, const std::vector<uint8_t>& data);
    FileOperationResult appendToFile(const std::string& path, const std::vector<uint8_t>& data);

    // File information
    FileInfo getFileInfo(const std::string& path);
    bool fileExists(const std::string& path);
    bool isDirectory(const std::string& path);
    bool isFile(const std::string& path);
    std::vector<FileInfo> listFiles(const std::string& path, FileListCallback callback = nullptr);
    std::vector<FileInfo> listFilesRecursive(const std::string& path);

    // Scoped storage paths
    std::string getAppSpecificDirectory(StorageScope scope) const;
    std::string getExternalFilesDir() const;
    std::string getExternalCacheDir() const;
    std::string getExternalMediaDir() const;
    std::string getInternalFilesDir() const;
    std::string getInternalCacheDir() const;

    // MediaStore operations
    std::string insertMediaFile(MediaFileType type, const std::string& filePath, const MediaFileMetadata& metadata);
    FileOperationResult deleteMediaFile(const std::string& mediaUri);
    std::vector<FileInfo> queryMediaFiles(MediaFileType type, const std::string& selection = "");
    MediaFileMetadata getMediaFileMetadata(const std::string& mediaUri);

    // SAF (Storage Access Framework) operations
    std::string openDocumentTree(const std::string& initialUri = "");
    std::string openDocument(const std::vector<std::string>& mimeTypes);
    std::vector<FileInfo> listSAFDocuments(const std::string& treeUri);
    FileOperationResult createSAFFile(const std::string& parentUri, const std::string& fileName, const std::string& mimeType);
    std::vector<uint8_t> readSAFDocument(const std::string& documentUri);
    FileOperationResult writeSAFDocument(const std::string& documentUri, const std::vector<uint8_t>& data);

    // Utility functions
    std::string getMimeType(const std::string& filePath);
    std::string getFileExtension(const std::string& filePath);
    bool isImageFile(const std::string& filePath);
    bool isVideoFile(const std::string& filePath);
    bool isAudioFile(const std::string& filePath);
    bool isDocumentFile(const std::string& filePath);

    // Migration helpers
    void migrateLegacyStorage();
    std::vector<std::string> getLegacyStoragePaths();
    bool isLegacyStorageAvailable();

private:
    void initializePermissions();
    void checkPermissions();
    void requestPermissionInternal(StoragePermission permission);
    void updateStorageVolumes();
    void startPermissionThread();
    void stopPermissionThread();
    void permissionThreadLoop();

    // JNI helper methods
    bool checkPermissionJNI(StoragePermission permission);
    void requestPermissionJNI(StoragePermission permission);
    std::vector<StorageVolumeInfo> getVolumesJNI();
    std::string getPathForScopeJNI(StorageScope scope);

    // File operation helpers
    FileOperationResult performFileOperation(const std::string& path, std::function<FileOperationResult()> operation);
    std::string sanitizeFileName(const std::string& fileName);
    bool isValidFileName(const std::string& fileName);
    std::string generateUniqueFileName(const std::string& directory, const std::string& baseName);

    // Callback management
    std::string generateCallbackId();
    void invokeStorageCallback(const std::string& callbackId, FileOperationResult result, const std::string& message);
    void invokeFileListCallback(const std::string& callbackId, FileOperationResult result, const std::vector<FileInfo>& files);
    void invokePermissionCallback(const std::string& callbackId, bool granted, StoragePermission permission);
    void invokeVolumeCallback(const std::string& callbackId, const std::vector<StorageVolumeInfo>& volumes);
};

// ========== MEDIA STORE MANAGER ==========
class MediaStoreManager {
private:
    ScopedStorageManager* storageManager_;

    // Media collections
    std::unordered_map<MediaFileType, std::string> collectionUris_;

    // Pending operations
    struct PendingMediaOperation {
        MediaFileType type;
        std::string filePath;
        MediaFileMetadata metadata;
        std::function<void(const std::string&)> callback;
    };

    std::vector<PendingMediaOperation> pendingOperations_;
    std::mutex operationsMutex_;

public:
    MediaStoreManager(ScopedStorageManager* storageManager);
    ~MediaStoreManager();

    bool initialize();
    void shutdown();

    // Media file operations
    std::string insertFile(MediaFileType type, const std::string& filePath, const MediaFileMetadata& metadata);
    FileOperationResult deleteFile(const std::string& mediaUri);
    FileOperationResult updateFile(const std::string& mediaUri, const MediaFileMetadata& metadata);

    // Media queries
    std::vector<FileInfo> queryFiles(MediaFileType type, const std::string& selection = "",
                                   const std::vector<std::string>& selectionArgs = {},
                                   const std::string& sortOrder = "");
    std::vector<FileInfo> queryImages(const std::string& bucketId = "");
    std::vector<FileInfo> queryVideos(const std::string& bucketId = "");
    std::vector<FileInfo> queryAudio(const std::string& bucketId = "");

    // Media metadata
    MediaFileMetadata getMetadata(const std::string& mediaUri);
    bool setMetadata(const std::string& mediaUri, const MediaFileMetadata& metadata);

    // Media buckets/albums
    std::vector<std::string> getBuckets(MediaFileType type);
    std::string getBucketPath(const std::string& bucketId);

    // Media scanning
    void scanFile(const std::string& filePath);
    void scanDirectory(const std::string& directory);

private:
    std::string getCollectionUri(MediaFileType type);
    std::string insertImage(const std::string& filePath, const MediaFileMetadata& metadata);
    std::string insertVideo(const std::string& filePath, const MediaFileMetadata& metadata);
    std::string insertAudio(const std::string& filePath, const MediaFileMetadata& metadata);
    std::string insertDocument(const std::string& filePath, const MediaFileMetadata& metadata);
    std::string insertDownload(const std::string& filePath, const MediaFileMetadata& metadata);
};

// ========== SAF MANAGER ==========
class SAFManager {
private:
    ScopedStorageManager* storageManager_;

    // Document trees
    struct DocumentTree {
        std::string treeUri;
        std::string rootPath;
        std::string documentId;
        bool persistent;
        time_t lastAccess;
    };

    std::unordered_map<std::string, DocumentTree> documentTrees_;
    std::mutex treesMutex_;

    // Open documents
    struct OpenDocument {
        std::string documentUri;
        std::string mimeType;
        int fileDescriptor;
        StorageAccessMode mode;
        time_t lastAccess;
    };

    std::unordered_map<std::string, OpenDocument> openDocuments_;
    std::mutex documentsMutex_;

public:
    SAFManager(ScopedStorageManager* storageManager);
    ~SAFManager();

    bool initialize();
    void shutdown();

    // Document tree operations
    std::string openDocumentTree(const std::string& initialUri = "");
    bool closeDocumentTree(const std::string& treeUri);
    std::vector<FileInfo> listDocuments(const std::string& treeUri);
    std::string createFile(const std::string& parentUri, const std::string& fileName, const std::string& mimeType);
    std::string createDirectory(const std::string& parentUri, const std::string& dirName);

    // Document operations
    std::string openDocument(const std::vector<std::string>& mimeTypes);
    bool closeDocument(const std::string& documentUri);
    std::vector<uint8_t> readDocument(const std::string& documentUri);
    FileOperationResult writeDocument(const std::string& documentUri, const std::vector<uint8_t>& data);
    FileOperationResult appendToDocument(const std::string& documentUri, const std::vector<uint8_t>& data);

    // Document information
    FileInfo getDocumentInfo(const std::string& documentUri);
    std::string getDocumentName(const std::string& documentUri);
    std::string getDocumentMimeType(const std::string& documentUri);
    std::string getDocumentParent(const std::string& documentUri);

    // Persistent permissions
    void takePersistableUriPermission(const std::string& uri, StorageAccessMode mode);
    void releasePersistableUriPermission(const std::string& uri);
    bool hasPersistableUriPermission(const std::string& uri, StorageAccessMode mode);

private:
    std::string convertDocumentUriToPath(const std::string& documentUri);
    std::string convertPathToDocumentUri(const std::string& path);
    void updateDocumentAccessTime(const std::string& documentUri);
    void cleanupExpiredDocuments();
};

// ========== JNI BRIDGE FUNCTIONS ==========
extern "C" {

    // Storage permission callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onPermissionResult(
        JNIEnv* env, jobject thiz, jstring permission, jboolean granted);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onAllPermissionsResult(
        JNIEnv* env, jobject thiz, jboolean allGranted);

    // Storage volume callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onVolumesEnumerated(
        JNIEnv* env, jobject thiz, jobjectArray volumes);

    // File operation callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onFileOperationResult(
        JNIEnv* env, jobject thiz, jstring operationId, jint result, jstring message);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onFileListResult(
        JNIEnv* env, jobject thiz, jstring operationId, jobjectArray files);

    // MediaStore callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onMediaFileInserted(
        JNIEnv* env, jobject thiz, jstring fileType, jstring fileUri);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onMediaFileDeleted(
        JNIEnv* env, jobject thiz, jstring fileUri, jboolean success);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onMediaFilesQueried(
        JNIEnv* env, jobject thiz, jstring fileType, jobjectArray files);

    // SAF callbacks
    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onDocumentTreeOpened(
        JNIEnv* env, jobject thiz, jstring treeUri, jstring rootPath);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onDocumentOpened(
        JNIEnv* env, jobject thiz, jstring documentUri, jstring mimeType);

    JNIEXPORT void JNICALL Java_com_foundryengine_game_ScopedStorageManager_onSAFFileOperationResult(
        JNIEnv* env, jobject thiz, jstring operationId, jstring documentUri, jint result);

}

} // namespace FoundryEngine

#endif // FOUNDRYENGINE_SCOPED_STORAGE_MANAGER_H
