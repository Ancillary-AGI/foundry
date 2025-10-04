/**
 * Windows System Implementation
 * Windows System Utilities and Platform Services
 */

#include "WindowsPlatform.h"
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <psapi.h>
#include <pdh.h>
#include <string>
#include <vector>
#include <fstream>

// Link required Windows libraries
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")

// ========== WINDOWS FILE SYSTEM ==========
class WindowsFileSystem : public PlatformFileSystem {
private:
    std::string appDataPath_;
    std::string documentsPath_;

public:
    WindowsFileSystem() {
        // Initialize paths
        appDataPath_ = getAppDataPath();
        documentsPath_ = getDocumentsPath();
    }

    std::vector<uint8_t> readFile(const std::string& path) override {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return {};
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);
        return data;
    }

    void writeFile(const std::string& path, const std::vector<uint8_t>& data) override {
        std::ofstream file(path, std::ios::binary);
        if (file) {
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
        }
    }

    void deleteFile(const std::string& path) override {
        DeleteFileA(path.c_str());
    }

    std::vector<std::string> listFiles(const std::string& directory) override {
        std::vector<std::string> files;
        std::string searchPath = directory + "\\*";

        WIN32_FIND_DATAA findData;
        HANDLE findHandle = FindFirstFileA(searchPath.c_str(), &findData);

        if (findHandle != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    files.push_back(findData.cFileName);
                }
            } while (FindNextFileA(findHandle, &findData));
            FindClose(findHandle);
        }

        return files;
    }

    void createDirectory(const std::string& path) override {
        CreateDirectoryA(path.c_str(), nullptr);
    }

    bool exists(const std::string& path) override {
        DWORD attributes = GetFileAttributesA(path.c_str());
        return attributes != INVALID_FILE_ATTRIBUTES;
    }

private:
    std::string getAppDataPath() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
            return std::string(path) + "\\FoundryEngine";
        }
        return ".\\data";
    }

    std::string getDocumentsPath() {
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, 0, path))) {
            return std::string(path) + "\\FoundryEngine";
        }
        return ".\\documents";
    }
};

// ========== WINDOWS TIMER ==========
class WindowsTimer : public PlatformTimer {
private:
    LARGE_INTEGER frequency_;
    LARGE_INTEGER startTime_;

public:
    WindowsTimer() {
        QueryPerformanceFrequency(&frequency_);
        QueryPerformanceCounter(&startTime_);
    }

    double now() override {
        LARGE_INTEGER current;
        QueryPerformanceCounter(&current);
        return static_cast<double>(current.QuadPart - startTime_.QuadPart) / frequency_.QuadPart * 1000.0;
    }

    int setTimeout(std::function<void()> callback, int delay) override {
        // Use Windows timers or std::thread
        return 0;
    }

    void clearTimeout(int id) override {
        // Clear timer
    }

    int setInterval(std::function<void()> callback, int delay) override {
        // Use Windows timers
        return 0;
    }

    void clearInterval(int id) override {
        // Clear interval
    }

    int requestAnimationFrame(std::function<void(double)> callback) override {
        // Windows doesn't have requestAnimationFrame, use timer
        return 0;
    }

    void cancelAnimationFrame(int id) override {
        // Cancel animation frame
    }
};

// ========== WINDOWS RANDOM ==========
class WindowsRandom : public PlatformRandom {
public:
    double random() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }

    int randomInt(int min, int max) override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }

    double randomFloat(double min, double max) override {
        return min + (random() * (max - min));
    }

    void seed(unsigned int seed) override {
        generator_.seed(seed);
    }

private:
    std::mt19937 generator_;
};
