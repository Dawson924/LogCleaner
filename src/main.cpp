#include <windows.h>
#include <shellapi.h>
#include "LogCleaner.h"
#include "LogCleanerConsole.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    ConsoleOptions options = {};
    bool runConsole = false;

    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--no-confirm") == 0) {
            options.noConfirm = true;
            runConsole = true;
        } else if (wcscmp(argv[i], L"-d") == 0 || wcscmp(argv[i], L"-D") == 0) {
            options.directDelete = true;
            runConsole = true;
        }
    }

    if (argv) {
        LocalFree(argv);
    }

    if (runConsole) {
        return RunConsole(options);
    }

    LogCleanerApp app;

    if (!app.Initialize(hInstance, nCmdShow)) {
        MessageBoxW(nullptr, L"Failed to initialize application.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    return app.Run();
}