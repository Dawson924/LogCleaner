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

    void CreateControls();
    void PopulateListView();
    void OnDelete();
    void OnCancel();
    void OnSelectAll();
    void OnDeselectAll();
    void RefreshListView();
    void ShowDeletedResults();

    HWND hwnd_;
    HWND hListView_;
    HWND hBtnDelete_;
    HWND hBtnCancel_;
    HWND hBtnSelectAll_;
    HWND hBtnDeselectAll_;
    HWND hChkSkipConfirmation_;
    HINSTANCE hInstance_;

    std::vector<LogFileInfo> logFiles_;
    std::vector<LogFileInfo> deletedFiles_;
    std::wstring iniPath_;
    AppSettings settings_;
    bool showingResults_;

    static constexpr int IDC_LISTVIEW = 1001;
    static constexpr int IDC_BTN_DELETE = 1002;
    static constexpr int IDC_BTN_CANCEL = 1003;
    static constexpr int IDC_BTN_SELECT_ALL = 1004;
    static constexpr int IDC_BTN_DESELECT_ALL = 1005;
    static constexpr int IDC_CHK_SKIP_CONFIRMATION = 1006;
};