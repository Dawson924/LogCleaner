#include "LogCleaner.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

LogCleanerApp::LogCleanerApp()
    : hwnd_(nullptr), hListView_(nullptr), hBtnDelete_(nullptr),
      hBtnCancel_(nullptr), hBtnSelectAll_(nullptr), hBtnDeselectAll_(nullptr),
      hChkSkipConfirmation_(nullptr), hInstance_(nullptr), showingResults_(false) {
}

LogCleanerApp::~LogCleanerApp() {
}

bool LogCleanerApp::Initialize(HINSTANCE hInstance, int nCmdShow) {
    hInstance_ = hInstance;
    iniPath_ = GetIniPath();
    settings_ = LoadSettings(iniPath_);

    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&iccex);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"LogCleanerWindowClass";
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 750;
    int windowHeight = 600;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    hwnd_ = CreateWindowExW(
        0,
        L"LogCleanerWindowClass",
        L"Log Cleaner",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, windowWidth, windowHeight,
        nullptr, nullptr, hInstance, this
    );

    if (!hwnd_) {
        return false;
    }

    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
    return true;
}

int LogCleanerApp::Run() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK LogCleanerApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LogCleanerApp* pApp = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pApp = static_cast<LogCleanerApp*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pApp));
        pApp->hwnd_ = hwnd;
    } else {
        pApp = reinterpret_cast<LogCleanerApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pApp) {
        return pApp->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT LogCleanerApp::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        CreateControls();
        return 0;

    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        SetWindowPos(hListView_, nullptr, 10, 10, width - 20, height - 120, SWP_NOZORDER);
        SetWindowPos(hBtnSelectAll_, nullptr, 10, height - 100, 100, 30, SWP_NOZORDER);
        SetWindowPos(hBtnDeselectAll_, nullptr, 120, height - 100, 100, 30, SWP_NOZORDER);
        SetWindowPos(hChkSkipConfirmation_, nullptr, width - 200, height - 100, 180, 30, SWP_NOZORDER);
        SetWindowPos(hBtnDelete_, nullptr, width - 220, height - 60, 100, 35, SWP_NOZORDER);
        SetWindowPos(hBtnCancel_, nullptr, width - 110, height - 60, 100, 35, SWP_NOZORDER);

        RECT rcList;
        GetClientRect(hListView_, &rcList);
        int listWidth = rcList.right - rcList.left;
        int col0Width = 200;
        int col2Width = 100;
        int col1Width = listWidth - col0Width - col2Width;
        if (col1Width < 100) col1Width = 100;
        ListView_SetColumnWidth(hListView_, 1, col1Width);

        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_DELETE:
            OnDelete();
            return 0;
        case IDC_BTN_CANCEL:
            OnCancel();
            return 0;
        case IDC_BTN_SELECT_ALL:
            OnSelectAll();
            return 0;
        case IDC_BTN_DESELECT_ALL:
            OnDeselectAll();
            return 0;
        }
        return 0;

    case WM_NOTIFY: {
        LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
        if (pnmh->idFrom == IDC_LISTVIEW && pnmh->code == LVN_ITEMCHANGED) {
            LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(lParam);
            if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_STATEIMAGEMASK)) {
                int index = pnmv->iItem;
                if (index >= 0 && index < static_cast<int>(logFiles_.size())) {
                    BOOL checked = ListView_GetCheckState(hListView_, index);
                    logFiles_[index].selected = (checked != FALSE);
                }
            }
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }
}

void LogCleanerApp::CreateControls() {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    hListView_ = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        10, 10, width - 20, height - 120,
        hwnd_, reinterpret_cast<HMENU>(IDC_LISTVIEW), hInstance_, nullptr
    );

    ListView_SetExtendedListViewStyle(hListView_, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES);

    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = const_cast<LPWSTR>(L"File Name");
    lvc.cx = 200;
    ListView_InsertColumn(hListView_, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = const_cast<LPWSTR>(L"Path");
    lvc.cx = 380;
    ListView_InsertColumn(hListView_, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = const_cast<LPWSTR>(L"Size");
    lvc.cx = 100;
    ListView_InsertColumn(hListView_, 2, &lvc);

    hBtnSelectAll_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Select All",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, height - 100, 100, 30,
        hwnd_, reinterpret_cast<HMENU>(IDC_BTN_SELECT_ALL), hInstance_, nullptr
    );

    hBtnDeselectAll_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Deselect All",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        120, height - 100, 100, 30,
        hwnd_, reinterpret_cast<HMENU>(IDC_BTN_DESELECT_ALL), hInstance_, nullptr
    );

    hChkSkipConfirmation_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Skip Confirmation",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        width - 200, height - 100, 180, 30,
        hwnd_, reinterpret_cast<HMENU>(IDC_CHK_SKIP_CONFIRMATION), hInstance_, nullptr
    );

    if (settings_.skipConfirmation) {
        SendMessage(hChkSkipConfirmation_, BM_SETCHECK, BST_CHECKED, 0);
    }

    hBtnDelete_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Delete",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        width - 220, height - 60, 100, 35,
        hwnd_, reinterpret_cast<HMENU>(IDC_BTN_DELETE), hInstance_, nullptr
    );

    hBtnCancel_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        width - 110, height - 60, 100, 35,
        hwnd_, reinterpret_cast<HMENU>(IDC_BTN_CANCEL), hInstance_, nullptr
    );

    std::wstring currentDir = GetExecutableDir();
    logFiles_ = FindLogFilesRecursive(currentDir);

    if (settings_.skipConfirmation && !logFiles_.empty()) {
        OnDelete();
    } else {
        PopulateListView();
    }
}

void LogCleanerApp::PopulateListView() {
    ListView_DeleteAllItems(hListView_);

    for (size_t i = 0; i < logFiles_.size(); ++i) {
        LVITEM lvi = {};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = static_cast<int>(i);
        lvi.lParam = static_cast<LPARAM>(i);
        lvi.pszText = const_cast<LPWSTR>(logFiles_[i].fileName.c_str());
        ListView_InsertItem(hListView_, &lvi);

        ListView_SetItemText(hListView_, static_cast<int>(i), 1, const_cast<LPWSTR>(logFiles_[i].fullPath.c_str()));
        std::wstring sizeStr = FormatFileSize(logFiles_[i].fileSize);
        ListView_SetItemText(hListView_, static_cast<int>(i), 2, const_cast<LPWSTR>(sizeStr.c_str()));

        ListView_SetCheckState(hListView_, static_cast<int>(i), TRUE);
    }
}

void LogCleanerApp::RefreshListView() {
    for (size_t i = 0; i < logFiles_.size(); ++i) {
        ListView_SetCheckState(hListView_, static_cast<int>(i), logFiles_[i].selected ? TRUE : FALSE);
    }
}

void LogCleanerApp::OnSelectAll() {
    for (auto& file : logFiles_) {
        file.selected = true;
    }
    RefreshListView();
}

void LogCleanerApp::OnDeselectAll() {
    for (auto& file : logFiles_) {
        file.selected = false;
    }
    RefreshListView();
}

void LogCleanerApp::OnDelete() {
    deletedFiles_.clear();

    for (const auto& file : logFiles_) {
        if (file.selected) {
            if (DeleteFileSecure(file.fullPath)) {
                deletedFiles_.push_back(file);
            }
        }
    }

    bool skipConfirmation = (SendMessage(hChkSkipConfirmation_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    settings_.skipConfirmation = skipConfirmation;
    SaveSettings(iniPath_, settings_);

    if (skipConfirmation && !showingResults_) {
        ShowDeletedResults();
    } else {
        std::wostringstream msg;
        msg << L"Deleted " << deletedFiles_.size() << L" log file(s).\n\n";
        for (const auto& file : deletedFiles_) {
            msg << file.fileName << L" (" << FormatFileSize(file.fileSize) << L")\n";
        }
        MessageBoxW(hwnd_, msg.str().c_str(), L"Deletion Complete", MB_OK | MB_ICONINFORMATION);
        PostQuitMessage(0);
    }
}

void LogCleanerApp::ShowDeletedResults() {
    showingResults_ = true;
    SetWindowTextW(hwnd_, L"LogCleaner - Deleted Files");

    logFiles_ = deletedFiles_;
    for (auto& file : logFiles_) {
        file.selected = false;
    }

    ListView_DeleteAllItems(hListView_);
    ListView_SetExtendedListViewStyle(hListView_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = const_cast<LPWSTR>(L"File Name");
    lvc.cx = 200;
    ListView_InsertColumn(hListView_, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = const_cast<LPWSTR>(L"Path");
    lvc.cx = 380;
    ListView_InsertColumn(hListView_, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = const_cast<LPWSTR>(L"Size");
    lvc.cx = 100;
    ListView_InsertColumn(hListView_, 2, &lvc);

    for (size_t i = 0; i < logFiles_.size(); ++i) {
        LVITEM lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = static_cast<int>(i);
        lvi.pszText = const_cast<LPWSTR>(logFiles_[i].fileName.c_str());
        ListView_InsertItem(hListView_, &lvi);

        ListView_SetItemText(hListView_, static_cast<int>(i), 1, const_cast<LPWSTR>(logFiles_[i].fullPath.c_str()));
        std::wstring sizeStr = FormatFileSize(logFiles_[i].fileSize);
        ListView_SetItemText(hListView_, static_cast<int>(i), 2, const_cast<LPWSTR>(sizeStr.c_str()));
    }

    ShowWindow(hBtnSelectAll_, SW_HIDE);
    ShowWindow(hBtnDeselectAll_, SW_HIDE);
    ShowWindow(hChkSkipConfirmation_, SW_HIDE);

    SetWindowTextW(hBtnDelete_, L"OK");
    SetWindowTextW(hBtnCancel_, L"Close");
}

void LogCleanerApp::OnCancel() {
    bool skipConfirmation = (SendMessage(hChkSkipConfirmation_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    settings_.skipConfirmation = skipConfirmation;
    SaveSettings(iniPath_, settings_);
    PostQuitMessage(0);
}