#include <windows.h>
#include "LogCleaner.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    LogCleanerApp app;

    if (!app.Initialize(hInstance, nCmdShow)) {
        MessageBoxW(nullptr, L"Failed to initialize application.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    return app.Run();
}