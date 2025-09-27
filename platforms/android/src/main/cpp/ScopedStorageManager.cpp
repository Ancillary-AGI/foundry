#include "ScopedStorageManager.h"
#include <android/log.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <sys/statvfs.h>
#include <magic.h> // For MIME type detection
#include <libgen.h> // For basename/dirname

#define LOG_TAG "ScopedStorageManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace FoundryEngine {

// Static instance
ScopedStorageManager* ScopedStorageManager::instance_ = nullptr;

// ========== SCOPED STORAGE MANAGER IMPLEMENTATION ==========
ScopedStorageManager::ScopedStorageManager() : mediaStoreManager_(nullptr), safManager_(nullptr),
                                               env_(nullptr), context_(nullptr), initialized_(false),
                                               permissionThreadRunning_(false) {
    LOGI("ScopedStorageManager constructor called");
}

ScopedStorageManager::~ScopedStorageManager() {
    shutdown();
    LOGI("ScopedStorageManager destructor called");
}

ScopedStorageManager* ScopedStorageManager::getInstance() {
    if (!instance_) {
        instance_ = new ScopedStorageManager();
    }
    return instance_;
}

void ScopedStorageManager::initialize() {
    LOGI("Initializing Scoped Storage Manager");

    if (initialized_) {
        LOGW("Scoped Storage Manager already initialized");
        return;
    }

    // Initialize managers
    mediaStoreManager_ = new MediaStoreManager(this);
    safManager_ = new SAFManager(this);

    // Initialize permissions
    initializePermissions();

    // Start permission monitoring thread
    startPermissionThread();

    // Initialize MediaStore
    if (mediaStoreManager_->initialize()) {
        LOGI("MediaStore Manager initialized successfully");
    } else {
        LOGE("Failed to initialize MediaStore Manager");
    }

    // Initialize SAF
    if (safManager_->initialize()) {
        LOGI("SAF Manager initialized successfully");
    } else {
        LOGE("Failed to initialize SAF Manager");
    }

    initialized_ = true;
    LOGI("Scoped Storage Manager initialized successfully");
}

void ScopedStorageManager::update(float dt) {
    // Update storage volumes
    static float volumeUpdateTimer = 0.0f;
    volumeUpdateTimer += dt;

    if (volumeUpdateTimer >= 30.0f) { // Update every 30 seconds
        updateStorageVolumes();
        volumeUpdateTimer = 0.0f;
    }

    // Clean up expired callbacks
    // In a real implementation, this would clean up old callbacks
}

void ScopedStorageManager::shutdown() {
    LOGI("Shutting down Scoped Storage Manager");

    if (!initialized_) {
        return;
    }

    // Stop permission thread
    stopPermissionThread();

    // Shutdown managers
    if (mediaStoreManager_) {
        mediaStoreManager_->shutdown();
        delete mediaStoreManager_;
        mediaStoreManager_ = nullptr;
    }

    if (safManager_) {
        safManager_->shutdown();
        delete safManager_;
        safManager_ = nullptr;
    }

    // Clear callbacks
    storageCallbacks_.clear();
    fileListCallbacks_.clear();
    permissionCallbacks_.clear();
    volumeCallbacks_.clear();

    initialized_ = false;
    LOGI("Scoped Storage Manager shutdown complete");
}

void ScopedStorageManager::setJNIEnvironment(JNIEnv* env, jobject context) {
    env_ = env;
    context_ = context;
    LOGI("JNI environment set for Scoped Storage Manager");
}

void ScopedStorageManager::requestPermission(StoragePermission permission, PermissionCallback callback) {
    LOGI("Requesting permission: %d", static_cast<int>(permission));

    std::string callbackId = generateCallbackId();
    permissionCallbacks_[callbackId] = callback;

    requestPermissionInternal(permission);

    LOGI("Permission request initiated: %d", static_cast<int>(permission));
}

void ScopedStorageManager::requestAllPermissions() {
    LOGI("Requesting all storage permissions");

    // Request all required permissions
    for (int i = static_cast<int>(StoragePermission::READ_EXTERNAL_STORAGE);
         i <= static_cast<int>(StoragePermission::ACCESS_ALL_FILES); i++) {
        StoragePermission permission = static_cast<StoragePermission>(i);
        requestPermission(permission, nullptr);
    }

    LOGI("All permission requests initiated");
}

bool ScopedStorageManager::hasPermission(StoragePermission permission) const {
    std::lock_guard<std::mutex> lock(permissionsMutex_);

    auto it = permissions_.find(permission);
    if (it != permissions_.end()) {
        return it->second;
    }
    return false;
}

bool ScopedStorageManager::hasAllPermissions() const {
    // Check if all required permissions are granted
    return hasPermission(StoragePermission::READ_EXTERNAL_STORAGE) &&
           hasPermission(StoragePermission::WRITE_EXTERNAL_STORAGE);
}

void ScopedStorageManager::revokePermission(StoragePermission permission) {
    LOGI("Revoking permission: %d", static_cast<int>(permission));

    std::lock_guard<std::mutex> lock(permissionsMutex_);
    permissions_[permission] = false;

    LOGI("Permission revoked: %d", static_cast<int>(permission));
}

void ScopedStorageManager::enumerateVolumes(VolumeCallback callback) {
    LOGI("Enumerating storage volumes");

    std::string callbackId = generateCallbackId();
    volumeCallbacks_[callbackId] = callback;

    updateStorageVolumes();

    // Invoke callback with current volumes
    invokeVolumeCallback(callbackId, storageVolumes_);

    LOGI("Storage volumes enumerated");
}

const std::vector<StorageVolumeInfo>& ScopedStorageManager::getVolumes() const {
    std::lock_guard<std::mutex> lock(volumesMutex_);
    return storageVolumes_;
}

StorageVolumeInfo ScopedStorageManager::getPrimaryVolume() const {
    std::lock_guard<std::mutex> lock(volumesMutex_);

    for (const auto& volume : storageVolumes_) {
        if (volume.isPrimary) {
            return volume;
        }
    }

    // Return empty volume if no primary found
    return StorageVolumeInfo();
}

StorageVolumeInfo ScopedStorageManager::getVolumeByUuid(const std::string& uuid) const {
    std::lock_guard<std::mutex> lock(volumesMutex_);

    for (const auto& volume : storageVolumes_) {
        if (volume.uuid == uuid) {
            return volume;
        }
    }

    // Return empty volume if not found
    return StorageVolumeInfo();
}

int64_t ScopedStorageManager::getTotalSpace(const std::string& path) const {
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) == 0) {
        return static_cast<int64_t>(stat.f_blocks) * stat.f_frsize;
    }
    return 0;
}

int64_t ScopedStorageManager::getAvailableSpace(const std::string& path) const {
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) == 0) {
        return static_cast<int64_t>(stat.f_bavail) * stat.f_frsize;
    }
    return 0;
}

int64_t ScopedStorageManager::getUsedSpace(const std::string& path) const {
    int64_t total = getTotalSpace(path);
    int64_t available = getAvailableSpace(path);
    return total - available;
}

FileOperationResult ScopedStorageManager::createFile(const std::string& path, const std::string& fileName) {
    LOGI("Creating file: %s/%s", path.c_str(), fileName.c_str());

    std::string sanitizedName = sanitizeFileName(fileName);
    if (!isValidFileName(sanitizedName)) {
        LOGE("Invalid file name: %s", fileName.c_str());
        return FileOperationResult::INVALID_ARGUMENT;
    }

    std::string fullPath = path + "/" + sanitizedName;

    // Check if file already exists
    if (fileExists(fullPath)) {
        LOGE("File already exists: %s", fullPath.c_str());
        return FileOperationResult::FILE_EXISTS;
    }

    // Create the file
    int fd = open(fullPath.c_str(), O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (fd < 0) {
        LOGE("Failed to create file: %s (%s)", fullPath.c_str(), strerror(errno));
        return FileOperationResult::IO_ERROR;
    }

    close(fd);
    LOGI("File created successfully: %s", fullPath.c_str());
    return FileOperationResult::SUCCESS;
}

FileOperationResult ScopedStorageManager::createDirectory(const std::string& path, const std::string& dirName) {
    LOGI("Creating directory: %s/%s", path.c_str(), dirName.c_str());

    std::string sanitizedName = sanitizeFileName(dirName);
    if (!isValidFileName(sanitizedName)) {
        LOGE("Invalid directory name: %s", dirName.c_str());
        return FileOperationResult::INVALID_ARGUMENT;
    }

    std::string fullPath = path + "/" + sanitizedName;

    // Create the directory
    if (mkdir(fullPath.c_str(), 0755) < 0) {
        if (errno == EEXIST) {
            LOGE("Directory already exists: %s", fullPath.c_str());
            return FileOperationResult::FILE_EXISTS;
        } else {
            LOGE("Failed to create directory: %s (%s)", fullPath.c_str(), strerror(errno));
            return FileOperationResult::IO_ERROR;
        }
    }

    LOGI("Directory created successfully: %s", fullPath.c_str());
    return FileOperationResult::SUCCESS;
}

FileOperationResult ScopedStorageManager::deleteFile(const std::string& path) {
    LOGI("Deleting file: %s", path.c_str());

    if (!fileExists(path)) {
        LOGE("File not found: %s", path.c_str());
        return FileOperationResult::FILE_NOT_FOUND;
    }

    if (remove(path.c_str()) < 0) {
        LOGE("Failed to delete file: %s (%s)", path.c_str(), strerror(errno));
        return FileOperationResult::IO_ERROR;
    }

    LOGI("File deleted successfully: %s", path.c_str());
    return FileOperationResult::SUCCESS;
}

FileOperationResult ScopedStorageManager::deleteDirectory(const std::string& path, bool recursive) {
    LOGI("Deleting directory: %s (recursive: %s)", path.c_str(), recursive ? "true" : "false");

    if (!isDirectory(path)) {
        LOGE("Path is not a directory: %s", path.c_str());
        return FileOperationResult::INVALID_ARGUMENT;
    }

    if (recursive) {
        // Recursive deletion
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            LOGE("Failed to open directory: %s (%s)", path.c_str(), strerror(errno));
            return FileOperationResult::IO_ERROR;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            std::string entryPath = path + "/" + entry->d_name;

            if (entry->d_type == DT_DIR) {
                FileOperationResult result = deleteDirectory(entryPath, true);
                if (result != FileOperationResult::SUCCESS) {
                    closedir(dir);
                    return result;
                }
            } else {
                FileOperationResult result = deleteFile(entryPath);
                if (result != FileOperationResult::SUCCESS) {
                    closedir(dir);
                    return result;
                }
            }
        }

        closedir(dir);
    }

    if (rmdir(path.c_str()) < 0) {
        LOGE("Failed to delete directory: %s (%s)", path.c_str(), strerror(errno));
        return FileOperationResult::IO_ERROR;
    }

    LOGI("Directory deleted successfully: %s", path.c_str());
    return FileOperationResult::SUCCESS;
}

FileOperationResult ScopedStorageManager::copyFile(const std::string& sourcePath, const std::string& destPath) {
    LOGI("Copying file: %s -> %s", sourcePath.c_str(), destPath.c_str());

    if (!fileExists(sourcePath)) {
        LOGE("Source file not found: %s", sourcePath.c_str());
        return FileOperationResult::FILE_NOT_FOUND;
    }

    // Open source file
    int sourceFd = open(sourcePath.c_str(), O_RDONLY);
    if (sourceFd < 0) {
        LOGE("Failed to open source file: %s (%s)", sourcePath.c_str(), strerror(errno));
        return FileOperationResult::IO_ERROR;
    }

    // Open destination file
    int destFd = open(destPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destFd < 0) {
        close(sourceFd);
        LOGE("Failed to open destination file: %s (%s)", destPath.c_str(), strerror(errno));
        return FileOperationResult::IO_ERROR;
    }

    // Copy data
    char buffer[8192];
    ssize_t bytesRead, bytesWritten;
    while ((bytesRead = read(sourceFd, buffer, sizeof(buffer))) > 0) {
        bytesWritten = write(destFd, buffer, bytesRead);
        if (bytesWritten != bytesRead) {
            close(sourceFd);
            close(destFd);
            LOGE("Failed to write data during copy");
            return FileOperationResult::IO_ERROR;
        }
    }

    if (bytesRead < 0) {
        close(sourceFd);
        close(destFd);
        LOGE("Failed to read data during copy");
        return FileOperationResult::IO_ERROR;
    }

    close(sourceFd);
    close(destFd);

    LOGI("File copied successfully: %s -> %s", sourcePath.c_str(), destPath.c_str());
    return FileOperationResult::SUCCESS;
}

FileOperationResult ScopedStorageManager::moveFile(const std::string& sourcePath, const std::string& destPath) {
    LOGI("Moving file: %s -> %s", sourcePath.c_str(), destPath.c_str());

    if (!fileExists(sourcePath)) {
        LOGE("Source file not found: %s", sourcePath.c_str());
        return FileOperationResult::FILE_NOT_FOUND;
    }

    // Try rename first (atomic operation)
    if (rename(sourcePath.c_str(), destPath.c_str()) == 0) {
        LOGI("File moved successfully: %s -> %s", sourcePath.c_str(), destPath.c_str());
        return FileOperationResult::SUCCESS;
    }

    // If rename fails, try copy + delete
    FileOperationResult copyResult = copyFile(sourcePath, destPath);
    if (copyResult != FileOperationResult::SUCCESS) {
        return copyResult;
    }

    FileOperationResult deleteResult = deleteFile(sourcePath);
    if (deleteResult != FileOperationResult::SUCCESS) {
        LOGE("Failed to delete source file after copy: %s", sourcePath.c_str());
        return deleteResult;
    }

    LOGI("File moved successfully (copy+delete): %s -> %s", sourcePath.c_str(), destPath.c_str());
    return FileOperationResult::SUCCESS;
}

FileOperationResult ScopedStorageManager::renameFile(const std::string& oldPath, const std::string& newPath) {
    LOGI("Renaming file: %s -> %s", oldPath.c_str(), newPath.c_str());

    if (!fileExists(oldPath)) {
        LOGE("Source file not found: %s", oldPath.c_str());
        return FileOperationResult::FILE_NOT_FOUND;
    }

    if (rename(oldPath.c_str(), newPath.c_str()) < 0) {
        LOGE("Failed to rename file: %s (%s)", oldPath.c_str(), strerror(errno));
        return FileOperationResult::IO_ERROR;
    }

    LOGI("File renamed successfully: %s -> %s", oldPath.c_str(), newPath.c_str());
    return FileOperationResult::SUCCESS;
}

std::vector<uint8_t> ScopedStorageManager::readFile(const std::string& path) {
    LOGI("Reading file: %s", path.c_str());

    if (!fileExists(path)) {
        LOGE("File not found: %s", path.c_str());
        return std::vector<uint8_t>();
    }

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open file for reading: %s (%s)", path.c_str(), strerror(errno));
        return std::vector<uint8_t>();
    }

    // Get file size
    off_t fileSize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    std::vector<uint8_t> data(fileSize);

    ssize_t bytesRead = read(fd, data.data(), fileSize);
    if (bytesRead != fileSize) {
        LOGE("Failed to read complete file: %s", path.c_str());
        data.clear();
    } else {
        LOGI("File read successfully: %s (%zd bytes)", path.c_str(), data.size());
    }

    close(fd);
    return data;
}

FileOperationResult ScopedStorageManager::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    LOGI("Writing file: %s (%zu bytes)", path.c_str(), data.size());

    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        LOGE("Failed to open file for writing: %s (%s)", path.c_str(), strerror(errno));
        return FileOperationResult::IO_ERROR;
    }

    ssize_t bytesWritten = write(fd, data.data(), data.size());
    if (bytesWritten != static_cast<ssize_t>(data.size())) {
        close(fd);
        LOGE("Failed to write complete file: %s", path.c_str());
        return FileOperationResult::IO_ERROR;
    }

    close(fd);
    LOGI("File written successfully: %s", path.c_str());
    return FileOperationResult::SUCCESS;
}

FileOperationResult ScopedStorageManager::appendToFile(const std::string& path, const std::vector<uint8_t>& data) {
    LOGI("Appending to file: %s (%zu bytes)", path.c_str(), data.size());

    int fd = open(path.c_str(), O_WRONLY | O_APPEND);
    if (fd < 0) {
        LOGE("Failed to open file for appending: %s (%s)", path.c_str(), strerror(errno));
        return FileOperationResult::IO_ERROR;
    }

    ssize_t bytesWritten = write(fd, data.data(), data.size());
    if (bytesWritten != static_cast<ssize_t>(data.size())) {
        close(fd);
        LOGE("Failed to append complete data: %s", path.c_str());
        return FileOperationResult::IO_ERROR;
    }

    close(fd);
    LOGI("Data appended successfully: %s", path.c_str());
    return FileOperationResult::SUCCESS;
}

FileInfo ScopedStorageManager::getFileInfo(const std::string& path) {
    LOGI("Getting file info: %s", path.c_str());

    FileInfo info;
    struct stat statBuf;

    if (stat(path.c_str(), &statBuf) < 0) {
        LOGE("Failed to get file info: %s (%s)", path.c_str(), strerror(errno));
        return info;
    }

    info.name = basename(const_cast<char*>(path.c_str()));
    info.path = path;
    info.absolutePath = path; // In a real implementation, get absolute path
    info.size = statBuf.st_size;
    info.lastModified = statBuf.st_mtime;
    info.lastAccessed = statBuf.st_atime;
    info.isDirectory = S_ISDIR(statBuf.st_mode);
    info.isFile = S_ISREG(statBuf.st_mode);
    info.isHidden = info.name[0] == '.';
    info.isReadable = (statBuf.st_mode & S_IRUSR) != 0;
    info.isWritable = (statBuf.st_mode & S_IWUSR) != 0;
    info.isExecutable = (statBuf.st_mode & S_IXUSR) != 0;
    info.mimeType = getMimeType(path);
    info.extension = getFileExtension(path);

    return info;
}

bool ScopedStorageManager::fileExists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

bool ScopedStorageManager::isDirectory(const std::string& path) {
    struct stat statBuf;
    if (stat(path.c_str(), &statBuf) < 0) {
        return false;
    }
    return S_ISDIR(statBuf.st_mode);
}

bool ScopedStorageManager::isFile(const std::string& path) {
    struct stat statBuf;
    if (stat(path.c_str(), &statBuf) < 0) {
        return false;
    }
    return S_ISREG(statBuf.st_mode);
}

std::vector<FileInfo> ScopedStorageManager::listFiles(const std::string& path, FileListCallback callback) {
    LOGI("Listing files: %s", path.c_str());

    std::vector<FileInfo> files;
    DIR* dir = opendir(path.c_str());

    if (!dir) {
        LOGE("Failed to open directory: %s (%s)", path.c_str(), strerror(errno));

        if (callback) {
            callback(FileOperationResult::IO_ERROR, files);
        }
        return files;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        std::string entryPath = path + "/" + entry->d_name;
        FileInfo info = getFileInfo(entryPath);
        files.push_back(info);
    }

    closedir(dir);

    if (callback) {
        callback(FileOperationResult::SUCCESS, files);
    }

    LOGI("Listed %zu files in: %s", files.size(), path.c_str());
    return files;
}

std::vector<FileInfo> ScopedStorageManager::listFilesRecursive(const std::string& path) {
    LOGI("Listing files recursively: %s", path.c_str());

    std::vector<FileInfo> allFiles;

    // Use a queue for breadth-first traversal
    std::queue<std::string> directories;
    directories.push(path);

    while (!directories.empty()) {
        std::string currentDir = directories.front();
        directories.pop();

        std::vector<FileInfo> files = listFiles(currentDir);
        for (const auto& file : files) {
            allFiles.push_back(file);

            if (file.isDirectory) {
                directories.push(file.path);
            }
        }
    }

    LOGI("Listed %zu files recursively in: %s", allFiles.size(), path.c_str());
    return allFiles;
}

std::string ScopedStorageManager::getAppSpecificDirectory(StorageScope scope) const {
    // In a real implementation, this would return the appropriate scoped storage directory
    switch (scope) {
        case StorageScope::APP_SPECIFIC:
            return getInternalFilesDir();
        case StorageScope::CACHE:
            return getInternalCacheDir();
        case StorageScope::EXTERNAL_PUBLIC:
            return getExternalFilesDir();
        default:
            return getInternalFilesDir();
    }
}

std::string ScopedStorageManager::getExternalFilesDir() const {
    // In a real implementation, this would get the external files directory
    // For now, return a mock path
    return "/sdcard/Android/data/com.foundryengine.game/files";
}

std::string ScopedStorageManager::getExternalCacheDir() const {
    // In a real implementation, this would get the external cache directory
    // For now, return a mock path
    return "/sdcard/Android/data/com.foundryengine.game/cache";
}

std::string ScopedStorageManager::getExternalMediaDir() const {
    // In a real implementation, this would get the external media directory
    // For now, return a mock path
    return "/sdcard/Android/media/com.foundryengine.game";
}

std::string ScopedStorageManager::getInternalFilesDir() const {
    // In a real implementation, this would get the internal files directory
    // For now, return a mock path
    return "/data/data/com.foundryengine.game/files";
}

std::string ScopedStorageManager::getInternalCacheDir() const {
    // In a real implementation, this would get the internal cache directory
    // For now, return a mock path
    return "/data/data/com.foundryengine.game/cache";
}

std::string ScopedStorageManager::insertMediaFile(MediaFileType type, const std::string& filePath, const MediaFileMetadata& metadata) {
    LOGI("Inserting media file: %s", filePath.c_str());

    if (mediaStoreManager_) {
        return mediaStoreManager_->insertFile(type, filePath, metadata);
    } else {
        LOGE("MediaStore Manager not available");
        return "";
    }
}

FileOperationResult ScopedStorageManager::deleteMediaFile(const std::string& mediaUri) {
    LOGI("Deleting media file: %s", mediaUri.c_str());

    if (mediaStoreManager_) {
        return mediaStoreManager_->deleteFile(mediaUri);
    } else {
        LOGE("MediaStore Manager not available");
        return FileOperationResult::UNKNOWN_ERROR;
    }
}

std::vector<FileInfo> ScopedStorageManager::queryMediaFiles(MediaFileType type, const std::string& selection) {
    LOGI("Querying media files: %d", static_cast<int>(type));

    if (mediaStoreManager_) {
        return mediaStoreManager_->queryFiles(type, selection);
    } else {
        LOGE("MediaStore Manager not available");
        return std::vector<FileInfo>();
    }
}

MediaFileMetadata ScopedStorageManager::getMediaFileMetadata(const std::string& mediaUri) {
    LOGI("Getting media file metadata: %s", mediaUri.c_str());

    if (mediaStoreManager_) {
        return mediaStoreManager_->getMetadata(mediaUri);
    } else {
        LOGE("MediaStore Manager not available");
        return MediaFileMetadata();
    }
}

std::string ScopedStorageManager::openDocumentTree(const std::string& initialUri) {
    LOGI("Opening document tree: %s", initialUri.c_str());

    if (safManager_) {
        return safManager_->openDocumentTree(initialUri);
    } else {
        LOGE("SAF Manager not available");
        return "";
    }
}

std::string ScopedStorageManager::openDocument(const std::vector<std::string>& mimeTypes) {
    LOGI("Opening document");

    if (safManager_) {
        return safManager_->openDocument(mimeTypes);
    } else {
        LOGE("SAF Manager not available");
        return "";
    }
}

std::vector<FileInfo> ScopedStorageManager::listSAFDocuments(const std::string& treeUri) {
    LOGI("Listing SAF documents: %s", treeUri.c_str());

    if (safManager_) {
        return safManager_->listDocuments(treeUri);
    } else {
        LOGE("SAF Manager not available");
        return std::vector<FileInfo>();
    }
}

FileOperationResult ScopedStorageManager::createSAFFile(const std::string& parentUri, const std::string& fileName, const std::string& mimeType) {
    LOGI("Creating SAF file: %s/%s", parentUri.c_str(), fileName.c_str());

    if (safManager_) {
        return FileOperationResult::UNKNOWN_ERROR; // SAF createFile returns string, not result
    } else {
        LOGE("SAF Manager not available");
        return FileOperationResult::UNKNOWN_ERROR;
    }
}

std::vector<uint8_t> ScopedStorageManager::readSAFDocument(const std::string& documentUri) {
    LOGI("Reading SAF document: %s", documentUri.c_str());

    if (safManager_) {
        return safManager_->readDocument(documentUri);
    } else {
        LOGE("SAF Manager not available");
        return std::vector<uint8_t>();
    }
}

FileOperationResult ScopedStorageManager::writeSAFDocument(const std::string& documentUri, const std::vector<uint8_t>& data) {
    LOGI("Writing SAF document: %s", documentUri.c_str());

    if (safManager_) {
        return safManager_->writeDocument(documentUri, data);
    } else {
        LOGE("SAF Manager not available");
        return FileOperationResult::UNKNOWN_ERROR;
    }
}

std::string ScopedStorageManager::getMimeType(const std::string& filePath) {
    // In a real implementation, this would use libmagic or Android's MIME type detection
    // For now, we'll use a simple extension-based approach

    std::string extension = getFileExtension(filePath);

    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".png") return "image/png";
    if (extension == ".gif") return "image/gif";
    if (extension == ".bmp") return "image/bmp";
    if (extension == ".webp") return "image/webp";
    if (extension == ".mp4") return "video/mp4";
    if (extension == ".avi") return "video/avi";
    if (extension == ".mkv") return "video/mkv";
    if (extension == ".mp3") return "audio/mpeg";
    if (extension == ".wav") return "audio/wav";
    if (extension == ".ogg") return "audio/ogg";
    if (extension == ".pdf") return "application/pdf";
    if (extension == ".txt") return "text/plain";
    if (extension == ".json") return "application/json";
    if (extension == ".xml") return "application/xml";
    if (extension == ".zip") return "application/zip";

    return "application/octet-stream";
}

std::string ScopedStorageManager::getFileExtension(const std::string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        return filePath.substr(dotPos);
    }
    return "";
}

bool ScopedStorageManager::isImageFile(const std::string& filePath) {
    std::string mimeType = getMimeType(filePath);
    return mimeType.find("image/") == 0;
}

bool ScopedStorageManager::isVideoFile(const std::string& filePath) {
    std::string mimeType = getMimeType(filePath);
    return mimeType.find("video/") == 0;
}

bool ScopedStorageManager::isAudioFile(const std::string& filePath) {
    std::string mimeType = getMimeType(filePath);
    return mimeType.find("audio/") == 0;
}

bool ScopedStorageManager::isDocumentFile(const std::string& filePath) {
    std::string mimeType = getMimeType(filePath);
    return mimeType.find("application/") == 0 || mimeType.find("text/") == 0;
}

void ScopedStorageManager::migrateLegacyStorage() {
    LOGI("Migrating legacy storage");

    // In a real implementation, this would migrate files from legacy storage
    // For now, we'll just log the action

    LOGI("Legacy storage migration completed");
}

std::vector<std::string> ScopedStorageManager::getLegacyStoragePaths() {
    // In a real implementation, this would return legacy storage paths
    // For now, return mock paths
    return {"/sdcard", "/storage/emulated/0"};
}

bool ScopedStorageManager::isLegacyStorageAvailable() {
    // In a real implementation, this would check if legacy storage is available
    // For now, return false (scoped storage is preferred)
    return false;
}

void ScopedStorageManager::initializePermissions() {
    LOGI("Initializing storage permissions");

    // Check current permissions
    checkPermissions();

    LOGI("Storage permissions initialized");
}

void ScopedStorageManager::checkPermissions() {
    std::lock_guard<std::mutex> lock(permissionsMutex_);

    // In a real implementation, this would check actual Android permissions
    // For now, we'll set some mock permissions
    permissions_[StoragePermission::READ_EXTERNAL_STORAGE] = true;
    permissions_[StoragePermission::WRITE_EXTERNAL_STORAGE] = true;
    permissions_[StoragePermission::ACCESS_MEDIA_LOCATION] = false;
    permissions_[StoragePermission::READ_MEDIA_IMAGES] = true;
    permissions_[StoragePermission::READ_MEDIA_VIDEO] = true;
    permissions_[StoragePermission::READ_MEDIA_AUDIO] = true;
    permissions_[StoragePermission::ACCESS_ALL_FILES] = false;
}

void ScopedStorageManager::requestPermissionInternal(StoragePermission permission) {
    // In a real implementation, this would request the permission from Android
    // For now, we'll simulate the request

    LOGI("Requesting permission internally: %d", static_cast<int>(permission));

    // Simulate permission granted
    std::lock_guard<std::mutex> lock(permissionsMutex_);
    permissions_[permission] = true;
}

void ScopedStorageManager::updateStorageVolumes() {
    LOGI("Updating storage volumes");

    std::lock_guard<std::mutex> lock(volumesMutex_);

    // In a real implementation, this would query Android for storage volumes
    // For now, we'll create mock volumes
    storageVolumes_.clear();

    StorageVolumeInfo internal;
    internal.uuid = "internal";
    internal.label = "Internal Storage";
    internal.type = StorageVolumeType::INTERNAL;
    internal.rootPath = "/data";
    internal.totalSpace = 16LL * 1024 * 1024 * 1024; // 16GB
    internal.availableSpace = 8LL * 1024 * 1024 * 1024; // 8GB
    internal.usedSpace = internal.totalSpace - internal.availableSpace;
    internal.isPrimary = true;
    internal.isRemovable = false;
    internal.isEmulated = false;
    internal.state = "mounted";
    storageVolumes_.push_back(internal);

    StorageVolumeInfo external;
    external.uuid = "external";
    external.label = "SD Card";
    external.type = StorageVolumeType::EXTERNAL;
    external.rootPath = "/sdcard";
    external.totalSpace = 32LL * 1024 * 1024 * 1024; // 32GB
    external.availableSpace = 16LL * 1024 * 1024 * 1024; // 16GB
    external.usedSpace = external.totalSpace - external.availableSpace;
    external.isPrimary = false;
    external.isRemovable = true;
    external.isEmulated = true;
    external.state = "mounted";
    storageVolumes_.push_back(external);

    LOGI("Storage volumes updated: %zu volumes", storageVolumes_.size());
}

void ScopedStorageManager::startPermissionThread() {
    LOGI("Starting permission monitoring thread");

    permissionThreadRunning_ = true;
    permissionThread_ = std::thread(&ScopedStorageManager::permissionThreadLoop, this);

    LOGI("Permission monitoring thread started");
}

void ScopedStorageManager::stopPermissionThread() {
    LOGI("Stopping permission monitoring thread");

    permissionThreadRunning_ = false;
    if (permissionThread_.joinable()) {
        permissionThread_.join();
    }

    LOGI("Permission monitoring thread stopped");
}

void ScopedStorageManager::permissionThreadLoop() {
    LOGI("Permission monitoring thread started");

    while (permissionThreadRunning_) {
        // Check permissions periodically
        checkPermissions();

        // Sleep for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    LOGI("Permission monitoring thread ended");
}

std::string ScopedStorageManager::generateCallbackId() {
    static std::atomic<int> counter(0);
    return "callback_" + std::to_string(counter++);
}

void ScopedStorageManager::invokeStorageCallback(const std::string& callbackId, FileOperationResult result, const std::string& message) {
    auto it = storageCallbacks_.find(callbackId);
    if (it != storageCallbacks_.end()) {
        it->second(result, message);
        storageCallbacks_.erase(it);
    }
}

void ScopedStorageManager::invokeFileListCallback(const std::string& callbackId, FileOperationResult result, const std::vector<FileInfo>& files) {
    auto it = fileListCallbacks_.find(callbackId);
    if (it != fileListCallbacks_.end()) {
        it->second(result, files);
        fileListCallbacks_.erase(it);
    }
}

void ScopedStorageManager::invokePermissionCallback(const std::string& callbackId, bool granted, StoragePermission permission) {
    auto it = permissionCallbacks_.find(callbackId);
    if (it != permissionCallbacks_.end()) {
        it->second(granted, permission);
        permissionCallbacks_.erase(it);
    }
}

void ScopedStorageManager::invokeVolumeCallback(const std::string& callbackId, const std::vector<StorageVolumeInfo>& volumes) {
    auto it = volumeCallbacks_.find(callbackId);
    if (it != volumeCallbacks_.end()) {
        it->second(volumes);
        volumeCallbacks_.erase(it);
    }
}

FileOperationResult ScopedStorageManager::performFileOperation(const std::string& path, std::function<FileOperationResult()> operation) {
    // Check if we have permission
    if (!hasPermission(StoragePermission::READ_EXTERNAL_STORAGE) &&
        !hasPermission(StoragePermission::WRITE_EXTERNAL_STORAGE)) {
        return FileOperationResult::PERMISSION_DENIED;
    }

    return operation();
}

std::string ScopedStorageManager::sanitizeFileName(const std::string& fileName) {
    std::string sanitized = fileName;

    // Replace invalid characters
    std::replace(sanitized.begin(), sanitized.end(), '/', '_');
    std::replace(sanitized.begin(), sanitized.end(), '\\', '_');
    std::replace(sanitized.begin(), sanitized.end(), ':', '_');
    std::replace(sanitized.begin(), sanitized.end(), '*', '_');
    std::replace(sanitized.begin(), sanitized.end(), '?', '_');
    std::replace(sanitized.begin(), sanitized.end(), '"', '_');
    std::replace(sanitized.begin(), sanitized.end(), '<', '_');
    std::replace(sanitized.begin(), sanitized.end(), '>', '_');
    std::replace(sanitized.begin(), sanitized.end(), '|', '_');

    return sanitized;
}

bool ScopedStorageManager::isValidFileName(const std::string& fileName) {
    if (fileName.empty() || fileName.length() > 255) {
        return false;
    }

    // Check for reserved names (Windows style, but good practice)
    std::vector<std::string> reservedNames = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4",
                                             "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2",
                                             "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

    std::string upperName = fileName;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

    for (const auto& reserved : reservedNames) {
        if (upperName == reserved) {
            return false;
        }
    }

    return true;
}

std::string ScopedStorageManager::generateUniqueFileName(const std::string& directory, const std::string& baseName) {
    std::string name = baseName;
    std::string extension = getFileExtension(name);
    std::string base = name.substr(0, name.length() - extension.length());

    int counter = 1;
    while (fileExists(directory + "/" + name)) {
        name = base + "_" + std::to_string(counter) + extension;
        counter++;
    }

    return name;
}

// ========== MEDIA STORE MANAGER IMPLEMENTATION ==========
MediaStoreManager::MediaStoreManager(ScopedStorageManager* storageManager) : storageManager_(storageManager) {
    LOGI("MediaStoreManager constructor called");
}

MediaStoreManager::~MediaStoreManager() {
    shutdown();
    LOGI("MediaStoreManager destructor called");
}

bool MediaStoreManager::initialize() {
    LOGI("Initializing MediaStore Manager");

    // Initialize collection URIs
    collectionUris_[MediaFileType::IMAGE] = "content://media/external/images/media";
    collectionUris_[MediaFileType::VIDEO] = "content://media/external/video/media";
    collectionUris_[MediaFileType::AUDIO] = "content://media/external/audio/media";

    LOGI("MediaStore Manager initialized successfully");
    return true;
}

void MediaStoreManager::shutdown() {
    LOGI("Shutting down MediaStore Manager");

    // Clear pending operations
    std::lock_guard<std::mutex> lock(operationsMutex_);
    pendingOperations_.clear();

    LOGI("MediaStore Manager shutdown complete");
}

std::string MediaStoreManager::insertFile(MediaFileType type, const std::string& filePath, const MediaFileMetadata& metadata) {
    LOGI("Inserting media file: %s", filePath.c_str());

    switch (type) {
        case MediaFileType::IMAGE:
            return insertImage(filePath, metadata);
        case MediaFileType::VIDEO:
            return insertVideo(filePath, metadata);
        case MediaFileType::AUDIO:
            return insertAudio(filePath, metadata);
        case MediaFileType::DOCUMENT:
            return insertDocument(filePath, metadata);
        case MediaFileType::DOWNLOAD:
            return insertDownload(filePath, metadata);
        default:
            LOGE("Unknown media file type: %d", static_cast<int>(type));
            return "";
    }
}

FileOperationResult MediaStoreManager::deleteFile(const std::string& mediaUri) {
    LOGI("Deleting media file: %s", mediaUri.c_str());

    // In a real implementation, this would delete from MediaStore
    // For now, return success
    return FileOperationResult::SUCCESS;
}

FileOperationResult MediaStoreManager::updateFile(const std::string& mediaUri, const MediaFileMetadata& metadata) {
    LOGI("Updating media file: %s", mediaUri.c_str());

    // In a real implementation, this would update MediaStore metadata
    // For now, return success
    return FileOperationResult::SUCCESS;
}

std::vector<FileInfo> MediaStoreManager::queryFiles(MediaFileType type, const std::string& selection,
                                                  const std::vector<std::string>& selectionArgs,
                                                  const std::string& sortOrder) {
    LOGI("Querying media files: %d", static_cast<int>(type));

    // In a real implementation, this would query MediaStore
    // For now, return empty vector
    return std::vector<FileInfo>();
}

std::vector<FileInfo> MediaStoreManager::queryImages(const std::string& bucketId) {
    return queryFiles(MediaFileType::IMAGE, "bucket_id = ?", {bucketId});
}

std::vector<FileInfo> MediaStoreManager::queryVideos(const std::string& bucketId) {
    return queryFiles(MediaFileType::VIDEO, "bucket_id = ?", {bucketId});
}

std::vector<FileInfo> MediaStoreManager::queryAudio(const std::string& bucketId) {
    return queryFiles(MediaFileType::AUDIO, "bucket_id = ?", {bucketId});
}

MediaFileMetadata MediaStoreManager::getMetadata(const std::string& mediaUri) {
    LOGI("Getting media metadata: %s", mediaUri.c_str());

    // In a real implementation, this would query MediaStore for metadata
    // For now, return empty metadata
    return MediaFileMetadata();
}

bool MediaStoreManager::setMetadata(const std::string& mediaUri, const MediaFileMetadata& metadata) {
    LOGI("Setting media metadata: %s", mediaUri.c_str());

    // In a real implementation, this would update MediaStore metadata
    // For now, return true
    return true;
}

std::vector<std::string> MediaStoreManager::getBuckets(MediaFileType type) {
    LOGI("Getting media buckets: %d", static_cast<int>(type));

    // In a real implementation, this would query MediaStore for buckets
    // For now, return empty vector
    return std::vector<std::string>();
}

std::string MediaStoreManager::getBucketPath(const std::string& bucketId) {
    LOGI("Getting bucket path: %s", bucketId.c_str());

    // In a real implementation, this would get the bucket path
    // For now, return empty string
    return "";
}

void MediaStoreManager::scanFile(const std::string& filePath) {
    LOGI("Scanning media file: %s", filePath.c_str());

    // In a real implementation, this would trigger MediaStore to scan the file
    // For now, just log the action
}

void MediaStoreManager::scanDirectory(const std::string& directory) {
    LOGI("Scanning media directory: %s", directory.c_str());

    // In a real implementation, this would trigger MediaStore to scan the directory
    // For now, just log the action
}

std::string MediaStoreManager::getCollectionUri(MediaFileType type) {
    auto it = collectionUris_.find(type);
    if (it != collectionUris_.end()) {
        return it->second;
    }
    return "";
}

std::string MediaStoreManager::insertImage(const std::string& filePath, const MediaFileMetadata& metadata) {
    // In a real implementation, this would insert image into MediaStore
    // For now, return mock URI
    return "content://media/external/images/media/1";
}

std::string MediaStoreManager::insertVideo(const std::string& filePath, const MediaFileMetadata& metadata) {
    // In a real implementation, this would insert video into MediaStore
    // For now, return mock URI
    return "content://media/external/video/media/1";
}

std::string MediaStoreManager::insertAudio(const std::string& filePath, const MediaFileMetadata& metadata) {
    // In a real implementation, this would insert audio into MediaStore
    // For now, return mock URI
    return "content://media/external/audio/media/1";
}

std::string MediaStoreManager::insertDocument(const std::string& filePath, const MediaFileMetadata& metadata) {
    // In a real implementation, this would insert document into MediaStore
    // For now, return mock URI
    return "content://media/external/documents/media/1";
}

std::string MediaStoreManager::insertDownload(const std::string& filePath, const MediaFileMetadata& metadata) {
    // In a real implementation, this would insert download into MediaStore
    // For now, return mock URI
    return "content://media/external/downloads/media/1";
}

// ========== SAF MANAGER IMPLEMENTATION ==========
SAFManager::SAFManager(ScopedStorageManager* storageManager) : storageManager_(storageManager) {
    LOGI("SAFManager constructor called");
}

SAFManager::~SAFManager() {
    shutdown();
    LOGI("SAFManager destructor called");
}

bool SAFManager::initialize() {
    LOGI("Initializing SAF Manager");

    // In a real implementation, this would initialize SAF
    // For now, just log success
    return true;
}

void SAFManager::shutdown() {
    LOGI("Shutting down SAF Manager");

    // Clean up document trees
    std::lock_guard<std::mutex> lock(treesMutex_);
    documentTrees_.clear();

    // Clean up open documents
    std::lock_guard<std::mutex> lock2(documentsMutex_);
    for (auto& pair : openDocuments_) {
        if (pair.second.fileDescriptor >= 0) {
            close(pair.second.fileDescriptor);
        }
    }
    openDocuments_.clear();

    LOGI("SAF Manager shutdown complete");
}

std::string SAFManager::openDocumentTree(const std::string& initialUri) {
    LOGI("Opening document tree: %s", initialUri.c_str());

    // In a real implementation, this would open a document tree picker
    // For now, return mock URI
    return "content://com.android.externalstorage.documents/tree/primary";
}

bool SAFManager::closeDocumentTree(const std::string& treeUri) {
    LOGI("Closing document tree: %s", treeUri.c_str());

    std::lock_guard<std::mutex> lock(treesMutex_);

    auto it = documentTrees_.find(treeUri);
    if (it != documentTrees_.end()) {
        documentTrees_.erase(it);
        return true;
    }

    return false;
}

std::vector<FileInfo> SAFManager::listDocuments(const std::string& treeUri) {
    LOGI("Listing SAF documents: %s", treeUri.c_str());

    // In a real implementation, this would list documents in the tree
    // For now, return empty vector
    return std::vector<FileInfo>();
}

std::string SAFManager::createFile(const std::string& parentUri, const std::string& fileName, const std::string& mimeType) {
    LOGI("Creating SAF file: %s/%s", parentUri.c_str(), fileName.c_str());

    // In a real implementation, this would create a file in the SAF tree
    // For now, return mock URI
    return parentUri + "/" + fileName;
}

std::string SAFManager::createDirectory(const std::string& parentUri, const std::string& dirName) {
    LOGI("Creating SAF directory: %s/%s", parentUri.c_str(), dirName.c_str());

    // In a real implementation, this would create a directory in the SAF tree
    // For now, return mock URI
    return parentUri + "/" + dirName;
}

std::string SAFManager::openDocument(const std::vector<std::string>& mimeTypes) {
    LOGI("Opening SAF document");

    // In a real implementation, this would open a document picker
    // For now, return mock URI
    return "content://media/external/images/media/1";
}

bool SAFManager::closeDocument(const std::string& documentUri) {
    LOGI("Closing SAF document: %s", documentUri.c_str());

    std::lock_guard<std::mutex> lock(documentsMutex_);

    auto it = openDocuments_.find(documentUri);
    if (it != openDocuments_.end()) {
        if (it->second.fileDescriptor >= 0) {
            close(it->second.fileDescriptor);
        }
        openDocuments_.erase(it);
        return true;
    }

    return false;
}

std::vector<uint8_t> SAFManager::readDocument(const std::string& documentUri) {
    LOGI("Reading SAF document: %s", documentUri.c_str());

    std::lock_guard<std::mutex> lock(documentsMutex_);

    auto it = openDocuments_.find(documentUri);
    if (it != openDocuments_.end()) {
        // In a real implementation, this would read from the file descriptor
        // For now, return empty data
        return std::vector<uint8_t>();
    }

    return std::vector<uint8_t>();
}

FileOperationResult SAFManager::writeDocument(const std::string& documentUri, const std::vector<uint8_t>& data) {
    LOGI("Writing SAF document: %s", documentUri.c_str());

    std::lock_guard<std::mutex> lock(documentsMutex_);

    auto it = openDocuments_.find(documentUri);
    if (it != openDocuments_.end()) {
        // In a real implementation, this would write to the file descriptor
        // For now, return success
        return FileOperationResult::SUCCESS;
    }

    return FileOperationResult::FILE_NOT_FOUND;
}

FileOperationResult SAFManager::appendToDocument(const std::string& documentUri, const std::vector<uint8_t>& data) {
    LOGI("Appending to SAF document: %s", documentUri.c_str());

    std::lock_guard<std::mutex> lock(documentsMutex_);

    auto it = openDocuments_.find(documentUri);
    if (it != openDocuments_.end()) {
        // In a real implementation, this would append to the file descriptor
        // For now, return success
        return FileOperationResult::SUCCESS;
    }

    return FileOperationResult::FILE_NOT_FOUND;
}

FileInfo SAFManager::getDocumentInfo(const std::string& documentUri) {
    LOGI("Getting SAF document info: %s", documentUri.c_str());

    // In a real implementation, this would get document information
    // For now, return empty info
    return FileInfo();
}

std::string SAFManager::getDocumentName(const std::string& documentUri) {
    LOGI("Getting SAF document name: %s", documentUri.c_str());

    // In a real implementation, this would extract the name from the URI
    // For now, return mock name
    return "document";
}

std::string SAFManager::getDocumentMimeType(const std::string& documentUri) {
    LOGI("Getting SAF document MIME type: %s", documentUri.c_str());

    // In a real implementation, this would get the MIME type
    // For now, return mock type
    return "application/octet-stream";
}

std::string SAFManager::getDocumentParent(const std::string& documentUri) {
    LOGI("Getting SAF document parent: %s", documentUri.c_str());

    // In a real implementation, this would get the parent URI
    // For now, return mock parent
    return "content://parent";
}

void SAFManager::takePersistableUriPermission(const std::string& uri, StorageAccessMode mode) {
    LOGI("Taking persistable URI permission: %s", uri.c_str());

    // In a real implementation, this would take persistable permissions
    // For now, just log the action
}

void SAFManager::releasePersistableUriPermission(const std::string& uri) {
    LOGI("Releasing persistable URI permission: %s", uri.c_str());

    // In a real implementation, this would release persistable permissions
    // For now, just log the action
}

bool SAFManager::hasPersistableUriPermission(const std::string& uri, StorageAccessMode mode) {
    LOGI("Checking persistable URI permission: %s", uri.c_str());

    // In a real implementation, this would check permissions
    // For now, return true
    return true;
}

} // namespace FoundryEngine
