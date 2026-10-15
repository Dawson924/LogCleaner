#include "Utils.h"
#include <shlwapi.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "shlwapi.lib")

std::vector<LogFileInfo> FindLogFilesRecursive(const std::wstring& rootDir) {
    std::vector<LogFileInfo> results;
    std::vector<std::wstring> dirsToProcess;
    dirsToProcess.push_back(rootDir);

    while (!dirsToProcess.empty()) {
        std::wstring currentDir = dirsToProcess.back();
        dirsToProcess.pop_back();

        std::wstring searchPath = currentDir;
        if (!searchPath.empty() && searchPath.back() != L'\\') {
            searchPath += L'\\';
        }
        searchPath += L"*.log";

        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                    LogFileInfo info;
                    info.fileName = findData.cFileName;
                    info.fullPath = currentDir;
                    if (!info.fullPath.empty() && info.fullPath.back() != L'\\') {
                        info.fullPath += L'\\';
                    }
                    info.fullPath += findData.cFileName;
                    ULARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    info.fileSize = fileSize.QuadPart;
                    info.selected = true;
                    results.push_back(info);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }

        std::wstring subDirSearch = currentDir;
        if (!subDirSearch.empty() && subDirSearch.back() != L'\\') {
            subDirSearch += L'\\';
        }
        subDirSearch += L"*";

        hFind = FindFirstFileW(subDirSearch.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                    if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
                        std::wstring subDir = currentDir;
                        if (!subDir.empty() && subDir.back() != L'\\') {
                            subDir += L'\\';
                        }
                        subDir += findData.cFileName;
                        dirsToProcess.push_back(subDir);
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    return results;
}

std::wstring FormatFileSize(ULONGLONG size) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB" };
    int unitIndex = 0;
    double displaySize = static_cast<double>(size);

    while (displaySize >= 1024.0 && unitIndex < 3) {
        displaySize /= 1024.0;
        ++unitIndex;
    }

    std::wostringstream oss;
    if (unitIndex == 0) {
        oss << size << L" " << units[unitIndex];
    } else {
        oss << std::fixed << std::setprecision(2) << displaySize << L" " << units[unitIndex];
    }
    return oss.str();
}

AppSettings LoadSettings(const std::wstring& iniPath) {
    AppSettings settings;
    settings.skipConfirmation = GetPrivateProfileIntW(L"Settings", L"SkipConfirmation", 0, iniPath.c_str()) != 0;
    return settings;
}

void SaveSettings(const std::wstring& iniPath, const AppSettings& settings) {
    WritePrivateProfileStringW(L"Settings", L"SkipConfirmation",
        settings.skipConfirmation ? L"1" : L"0", iniPath.c_str());
}

std::wstring GetExecutableDir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    return std::wstring(path);
}

std::wstring GetIniPath() {
    std::wstring exeDir = GetExecutableDir();
    if (!exeDir.empty() && exeDir.back() != L'\\') {
        exeDir += L'\\';
    }
    return exeDir + L"LogCleaner.ini";
}

bool DeleteFileSecure(const std::wstring& path) {
    return DeleteFileW(path.c_str()) != FALSE;
}