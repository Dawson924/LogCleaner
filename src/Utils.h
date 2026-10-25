#pragma once

#include <string>
#include <vector>
#include <windows.h>

struct LogFileInfo {
    std::wstring fullPath;
    std::wstring fileName;
    ULONGLONG fileSize;
    bool selected;
};

struct AppSettings {
    bool autoDeletion;
    bool directDelete;
};

std::vector<LogFileInfo> FindLogFilesRecursive(const std::wstring& rootDir);
std::wstring FormatFileSize(ULONGLONG size);
AppSettings LoadSettings(const std::wstring& iniPath);
void SaveSettings(const std::wstring& iniPath, const AppSettings& settings);
std::wstring GetExecutableDir();
std::wstring GetIniPath();
bool DeleteFileSecure(const std::wstring& path);