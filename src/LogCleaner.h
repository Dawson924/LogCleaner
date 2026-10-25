#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "Utils.h"

class LogCleanerApp {
public:
    LogCleanerApp();
    ~LogCleanerApp();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    int Run();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void CreateMenuBar();
    void CreateControls();
    void PopulateListView();
    void OnDelete();
    void OnSelectAll();
    void RefreshListView();
    void ShowDeletedResults();

    HWND hwnd_;
    HWND hListView_;
    HWND hBtnDelete_;
    HWND hChkSelectAll_;
    HWND hChkDirectDelete_;
    HINSTANCE hInstance_;
    HFONT hFontNormal_;
    HMENU hMenu_;
    HBRUSH hBottomBarBrush_;

    std::vector<LogFileInfo> logFiles_;
    std::vector<LogFileInfo> deletedFiles_;
    std::wstring iniPath_;
    AppSettings settings_;
    bool showingResults_;

    static constexpr int IDC_LISTVIEW = 1001;
    static constexpr int IDC_BTN_DELETE = 1002;
    static constexpr int IDC_CHK_SELECT_ALL = 1004;
    static constexpr int IDC_CHK_DIRECT_DELETE = 1005;
    static constexpr int IDM_CHK_AUTO_DELETION = 2001;

    static constexpr int PADDING = 12;
    static constexpr int BOTTOM_BAR_HEIGHT = 46;
    static constexpr int BTN_WIDTH = 80;
    static constexpr int BTN_HEIGHT = 28;
    static constexpr int CHECK_WIDTH = 120;
    static constexpr int WINDOW_WIDTH = 520;
    static constexpr int WINDOW_HEIGHT = 620;

    static constexpr COLORREF BOTTOM_BAR_COLOR = RGB(235, 238, 242);
    static constexpr COLORREF SEP_COLOR = RGB(220, 224, 230);
};