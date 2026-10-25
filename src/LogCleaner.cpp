#include "LogCleaner.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <shellapi.h>
#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

LogCleanerApp::LogCleanerApp()
    : hwnd_(nullptr), hListView_(nullptr), hBtnDelete_(nullptr),
      hChkSelectAll_(nullptr),
      hChkDirectDelete_(nullptr), hInstance_(nullptr),
      hFontNormal_(nullptr), hMenu_(nullptr),
      hBottomBarBrush_(nullptr), showingResults_(false) {
}

LogCleanerApp::~LogCleanerApp() {
    if (hFontNormal_) DeleteObject(hFontNormal_);
    if (hBottomBarBrush_) DeleteObject(hBottomBarBrush_);
}

bool LogCleanerApp::Initialize(HINSTANCE hInstance, int nCmdShow) {
    hInstance_ = hInstance;
    iniPath_ = GetIniPath();
    settings_ = LoadSettings(iniPath_);

    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&iccex);

    hBottomBarBrush_ = CreateSolidBrush(BOTTOM_BAR_COLOR);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_INFORMATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"LogCleanerWindowClass";
    wc.hIconSm = LoadIcon(nullptr, IDI_INFORMATION);

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - WINDOW_WIDTH) / 2;
    int y = (screenHeight - WINDOW_HEIGHT) / 2;

    CreateMenuBar();

    hwnd_ = CreateWindowExW(
        0,
        L"LogCleanerWindowClass",
        L"Log Cleaner",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, hMenu_, hInstance, this
    );

    if (!hwnd_) {
        return false;
    }

    MARGINS margins = { 0, 0, 0, 1 };
    DwmExtendFrameIntoClientArea(hwnd_, &margins);

    NONCLIENTMETRICSW ncm = {};
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    LOGFONTW lf = ncm.lfMessageFont;
    lf.lfHeight = -12;
    lf.lfWeight = FW_NORMAL;
    wcscpy_s(lf.lfFaceName, L"Segoe UI");
    hFontNormal_ = CreateFontIndirectW(&lf);

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

void LogCleanerApp::CreateMenuBar() {
    hMenu_ = CreateMenu();

    HMENU hMenu_1 = CreatePopupMenu();
    AppendMenuW(hMenu_1, MF_STRING | (settings_.autoDeletion ? MF_CHECKED : MF_UNCHECKED), IDM_CHK_AUTO_DELETION, L"Auto Deletion");

    AppendMenuW(hMenu_, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenu_1), L"Edit");
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

    case WM_CTLCOLORSTATIC: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND hwndCtrl = reinterpret_cast<HWND>(lParam);
        if (hwndCtrl == hChkSelectAll_ || hwndCtrl == hChkDirectDelete_) {
            SetBkMode(hdc, TRANSPARENT);
            SetBkColor(hdc, BOTTOM_BAR_COLOR);
            return reinterpret_cast<LRESULT>(hBottomBarBrush_);
        }
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }

    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        int listHeight = height - BOTTOM_BAR_HEIGHT;
        SetWindowPos(hListView_, nullptr, 0, 0, width, listHeight, SWP_NOZORDER);

        int checkY = height - BOTTOM_BAR_HEIGHT + (BOTTOM_BAR_HEIGHT - 22) / 2;

        SetWindowPos(hChkSelectAll_, nullptr, PADDING, checkY, CHECK_WIDTH, 22, SWP_NOZORDER);
        SetWindowPos(hChkDirectDelete_, nullptr, PADDING + CHECK_WIDTH + 4, checkY, CHECK_WIDTH, 22, SWP_NOZORDER);

        int btnY = height - BOTTOM_BAR_HEIGHT + (BOTTOM_BAR_HEIGHT - BTN_HEIGHT) / 2;
        int deleteX = width - PADDING - BTN_WIDTH;
        SetWindowPos(hBtnDelete_, nullptr, deleteX, btnY, BTN_WIDTH, BTN_HEIGHT, SWP_NOZORDER);

        RECT rcList;
        GetClientRect(hListView_, &rcList);
        int listWidth = rcList.right - rcList.left;
        int col0Width = 150;
        int col2Width = 80;
        int col1Width = listWidth - col0Width - col2Width - GetSystemMetrics(SM_CXVSCROLL);
        if (col1Width < 100) col1Width = 100;
        ListView_SetColumnWidth(hListView_, 1, col1Width);
        ListView_SetColumnWidth(hListView_, 2, LVSCW_AUTOSIZE_USEHEADER);

        return 0;
    }

    case WM_INITMENUPOPUP: {
        HMENU hPopup = reinterpret_cast<HMENU>(wParam);
        CheckMenuItem(hPopup, IDM_CHK_AUTO_DELETION, settings_.autoDeletion ? MF_CHECKED : MF_UNCHECKED);
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd_, &ps);

        RECT rc;
        GetClientRect(hwnd_, &rc);
        RECT bottomBar = rc;
        bottomBar.top = rc.bottom - BOTTOM_BAR_HEIGHT;
        FillRect(hdc, &bottomBar, hBottomBarBrush_);

        HPEN hPen = CreatePen(PS_SOLID, 1, SEP_COLOR);
        HPEN hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, hPen));
        MoveToEx(hdc, bottomBar.left, bottomBar.top, nullptr);
        LineTo(hdc, bottomBar.right, bottomBar.top);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        EndPaint(hwnd_, &ps);
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_CHK_AUTO_DELETION:
            settings_.autoDeletion = !settings_.autoDeletion;
            CheckMenuItem(GetMenu(hwnd_), IDM_CHK_AUTO_DELETION, settings_.autoDeletion ? MF_CHECKED : MF_UNCHECKED);
            SaveSettings(iniPath_, settings_);
            return 0;
        case IDC_BTN_DELETE:
            OnDelete();
            return 0;
        case IDC_CHK_SELECT_ALL:
            OnSelectAll();
            return 0;
        case IDC_CHK_DIRECT_DELETE:
            settings_.directDelete = (SendMessage(hChkDirectDelete_, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveSettings(iniPath_, settings_);
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
        if (hChkDirectDelete_) {
            settings_.directDelete = (SendMessage(hChkDirectDelete_, BM_GETCHECK, 0, 0) == BST_CHECKED);
        }
        SaveSettings(iniPath_, settings_);
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

    int listHeight = height - BOTTOM_BAR_HEIGHT;

    hListView_ = CreateWindowExW(
        0,
        WC_LISTVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        0, 0, width, listHeight,
        hwnd_, reinterpret_cast<HMENU>(IDC_LISTVIEW), hInstance_, nullptr
    );

    ListView_SetExtendedListViewStyle(hListView_, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    SetWindowTheme(hListView_, L"Explorer", nullptr);

    SendMessage(hListView_, WM_SETFONT, reinterpret_cast<WPARAM>(hFontNormal_), TRUE);

    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = const_cast<LPWSTR>(L"File Name");
    lvc.cx = 150;
    ListView_InsertColumn(hListView_, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = const_cast<LPWSTR>(L"Path");
    lvc.cx = width - 150 - 80 - GetSystemMetrics(SM_CXVSCROLL) - 4;
    ListView_InsertColumn(hListView_, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = const_cast<LPWSTR>(L"Size");
    lvc.cx = 80;
    ListView_InsertColumn(hListView_, 2, &lvc);

    int checkY = height - BOTTOM_BAR_HEIGHT + (BOTTOM_BAR_HEIGHT - 22) / 2;

    hChkSelectAll_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Select All",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        PADDING, checkY, CHECK_WIDTH, 22,
        hwnd_, reinterpret_cast<HMENU>(IDC_CHK_SELECT_ALL), hInstance_, nullptr
    );

    SendMessage(hChkSelectAll_, BM_SETCHECK, BST_CHECKED, 0);

    hChkDirectDelete_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Delete Directly",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        PADDING + CHECK_WIDTH + 4, checkY, CHECK_WIDTH, 22,
        hwnd_, reinterpret_cast<HMENU>(IDC_CHK_DIRECT_DELETE), hInstance_, nullptr
    );

    SendMessage(hChkDirectDelete_, BM_SETCHECK, settings_.directDelete ? BST_CHECKED : BST_UNCHECKED, 0);

    SendMessage(hChkSelectAll_, WM_SETFONT, reinterpret_cast<WPARAM>(hFontNormal_), TRUE);
    SendMessage(hChkDirectDelete_, WM_SETFONT, reinterpret_cast<WPARAM>(hFontNormal_), TRUE);

    int btnY = height - BOTTOM_BAR_HEIGHT + (BOTTOM_BAR_HEIGHT - BTN_HEIGHT) / 2;
    int deleteX = width - PADDING - BTN_WIDTH;

    hBtnDelete_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Delete",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_GROUP,
        deleteX, btnY, BTN_WIDTH, BTN_HEIGHT,
        hwnd_, reinterpret_cast<HMENU>(IDC_BTN_DELETE), hInstance_, nullptr
    );

    SendMessage(hBtnDelete_, WM_SETFONT, reinterpret_cast<WPARAM>(hFontNormal_), TRUE);

    std::wstring currentDir = GetExecutableDir();
    logFiles_ = FindLogFilesRecursive(currentDir);

    if (settings_.autoDeletion && !logFiles_.empty()) {
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
    bool selectAll = (SendMessage(hChkSelectAll_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    for (auto& file : logFiles_) {
        file.selected = selectAll;
    }
    RefreshListView();
}

void LogCleanerApp::OnDelete() {
    deletedFiles_.clear();

    bool directDelete = (SendMessage(hChkDirectDelete_, BM_GETCHECK, 0, 0) == BST_CHECKED);
    settings_.directDelete = directDelete;

    if (directDelete) {
        for (const auto& file : logFiles_) {
            if (file.selected) {
                if (DeleteFileSecure(file.fullPath)) {
                    deletedFiles_.push_back(file);
                }
            }
        }
    } else {
        std::wstring fileList;
        for (const auto& file : logFiles_) {
            if (file.selected) {
                fileList += file.fullPath;
                fileList.push_back(L'\0');
            }
        }
        if (!fileList.empty()) {
            fileList.push_back(L'\0');
            SHFILEOPSTRUCTW shfos = {};
            shfos.wFunc = FO_DELETE;
            shfos.pFrom = fileList.c_str();
            shfos.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
            if (SHFileOperationW(&shfos) == 0) {
                for (const auto& file : logFiles_) {
                    if (file.selected) {
                        deletedFiles_.push_back(file);
                    }
                }
            }
        }
    }

    SaveSettings(iniPath_, settings_);

    if (settings_.autoDeletion && !showingResults_) {
        ShowDeletedResults();
    } else {
        if (deletedFiles_.empty()) {
            MessageBoxW(hwnd_, L"No log files were found to delete.", L"Log Cleaner", MB_OK | MB_ICONINFORMATION);
        } else {
            std::wostringstream msg;
            msg << L"Successfully deleted " << deletedFiles_.size() << L" log file(s).\n\n";
            for (const auto& file : deletedFiles_) {
                msg << L"  \x2022  " << file.fileName << L"  (" << FormatFileSize(file.fileSize) << L")\n";
            }
            MessageBoxW(hwnd_, msg.str().c_str(), L"Deletion Complete", MB_OK | MB_ICONINFORMATION);
        }
        PostQuitMessage(0);
    }
}

void LogCleanerApp::ShowDeletedResults() {
    showingResults_ = true;
    SetWindowTextW(hwnd_, L"Log Cleaner - Deleted Files");

    logFiles_ = deletedFiles_;
    for (auto& file : logFiles_) {
        file.selected = false;
    }

    ListView_DeleteAllItems(hListView_);
    ListView_SetExtendedListViewStyle(hListView_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    LVCOLUMN lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    lvc.pszText = const_cast<LPWSTR>(L"File Name");
    lvc.cx = 150;
    ListView_InsertColumn(hListView_, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = const_cast<LPWSTR>(L"Path");
    lvc.cx = 200;
    ListView_InsertColumn(hListView_, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = const_cast<LPWSTR>(L"Size");
    lvc.cx = 80;
    ListView_InsertColumn(hListView_, 2, &lvc);

    ListView_SetColumnWidth(hListView_, 2, LVSCW_AUTOSIZE_USEHEADER);

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

    ShowWindow(hChkSelectAll_, SW_HIDE);
    ShowWindow(hChkDirectDelete_, SW_HIDE);

    SetWindowTextW(hBtnDelete_, L"OK");
}