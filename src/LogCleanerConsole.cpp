#include "LogCleanerConsole.h"
#include "Utils.h"
#include <windows.h>
#include <shellapi.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "shell32.lib")

static bool SetupConsole() {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
        if (!AllocConsole()) {
            return false;
        }
    }
    FILE* fpOut = nullptr;
    freopen_s(&fpOut, "CONOUT$", "w", stdout);
    FILE* fpIn = nullptr;
    freopen_s(&fpIn, "CONIN$", "r", stdin);
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);
    return true;
}

static std::wstring ReadLine() {
    std::wstring line;
    std::getline(std::wcin, line);
    return line;
}

int RunConsole(const ConsoleOptions& options) {
    if (!SetupConsole()) {
        return 1;
    }

    std::wcout << L"Log Cleaner - CLI Mode\n";
    std::wcout << L"----------------------\n";

    std::wstring currentDir = GetExecutableDir();
    std::vector<LogFileInfo> logFiles = FindLogFilesRecursive(currentDir);

    if (logFiles.empty()) {
        std::wcout << L"No .log files found in: " << currentDir << L"\n";
        return 0;
    }

    std::wcout << L"Found " << logFiles.size() << L" log file(s) in: " << currentDir << L"\n\n";
    for (size_t i = 0; i < logFiles.size(); ++i) {
        std::wcout << L"  [" << (i + 1) << L"] " << logFiles[i].fileName
                   << L"  (" << FormatFileSize(logFiles[i].fileSize) << L")\n";
    }
    std::wcout << L"\n";

    std::vector<int> selected;
    for (size_t i = 0; i < logFiles.size(); ++i) {
        selected.push_back(static_cast<int>(i));
    }
    std::wcout << L"Selected all " << logFiles.size() << L" file(s).\n";

    if (!options.noConfirm) {
        std::wcout << L"\nPress Enter to confirm deletion, or type 'n' to cancel: ";
        std::wstring confirm = ReadLine();
        std::wstring lower = confirm;
        for (auto& ch : lower) ch = static_cast<wchar_t>(towlower(ch));
        if (lower.find(L'n') != std::wstring::npos) {
            std::wcout << L"Cancelled.\n";
            return 0;
        }
    }

    std::vector<LogFileInfo> deleted;
    if (options.directDelete) {
        std::wcout << L"\nDeleting directly (no recycle bin)...\n";
        for (int idx : selected) {
            if (DeleteFileSecure(logFiles[idx].fullPath)) {
                deleted.push_back(logFiles[idx]);
            }
        }
    } else {
        std::wcout << L"\nMoving to recycle bin...\n";
        std::wstring fileList;
        for (int idx : selected) {
            fileList += logFiles[idx].fullPath;
            fileList.push_back(L'\0');
        }
        if (!fileList.empty()) {
            fileList.push_back(L'\0');
            SHFILEOPSTRUCTW shfos = {};
            shfos.wFunc = FO_DELETE;
            shfos.pFrom = fileList.c_str();
            shfos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
            if (SHFileOperationW(&shfos) == 0) {
                for (int idx : selected) {
                    deleted.push_back(logFiles[idx]);
                }
            }
        }
    }

    std::wcout << L"\nSuccessfully deleted " << deleted.size() << L" file(s):\n";
    for (const auto& file : deleted) {
        std::wcout << L"  \x2022  " << file.fileName
                   << L"  (" << FormatFileSize(file.fileSize) << L")\n";
    }

    return 0;
}