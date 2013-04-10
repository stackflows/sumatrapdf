/* Copyright 2013 the SumatraPDF project authors (see AUTHORS file).
   License: BSD */

#include "BaseUtil.h"

#include "FileUtil.h"
#include "Mui.h"
#include "Resource.h"
#include "TxtParser.h"
#include "WinUtil.h"

using namespace mui;

#define FRAME_CLASS_NAME    L"MUI_TEST_FRAME"

#define WIN_DX    720
#define WIN_DY    640

static HINSTANCE            ghinst = NULL;
static HWND                 gHwndFrame = NULL;
mui::HwndWrapper *gMainWnd = NULL;

static bool gShowTextBoundingBoxes = false;

#define TEN_SECONDS_IN_MS 10*1000

static float gUiDPIFactor = 1.0f;
inline int dpiAdjust(int value)
{
    return (int)(value * gUiDPIFactor);
}

static void OnExit()
{
    SendMessage(gHwndFrame, WM_CLOSE, 0, 0);
}

static inline void EnableAndShow(HWND hwnd, bool enable)
{
    ShowWindow(hwnd, enable ? SW_SHOW : SW_HIDE);
    EnableWindow(hwnd, enable);
}

static void OnOpen(HWND /* hwnd */)
{
}

#if 0
static void OnOpen(HWND hwnd)
{
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;

    str::Str<WCHAR> fileFilter;
    fileFilter.Append(L"All supported documents");

    ofn.lpstrFilter = L"All supported documents\0;*.mobi;*.awz;\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    ofn.nMaxFile = MAX_PATH * 100;
    ScopedMem<WCHAR> file(AllocArray<WCHAR>(ofn.nMaxFile));
    ofn.lpstrFile = file;

    if (!GetOpenFileName(&ofn))
        return;

    WCHAR *fileName = ofn.lpstrFile + ofn.nFileOffset;
    if (*(fileName - 1)) {
        // special case: single filename without NULL separator
        gEbookController->LoadMobi(ofn.lpstrFile);
        return;
    }
    // this is just a test app, no need to support multiple files
    CrashIf(true);
}
#endif

static void OnToggleBbox(HWND hwnd)
{
    gShowTextBoundingBoxes = !gShowTextBoundingBoxes;
    SetDebugPaint(gShowTextBoundingBoxes);
    InvalidateRect(hwnd, NULL, FALSE);
}

static LRESULT OnCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);

    if ((IDM_EXIT == wmId) || (IDCANCEL == wmId)) {
        OnExit();
        return 0;
    }

    if (IDM_OPEN == wmId) {
        OnOpen(hwnd);
        return 0;
    }

    if (IDM_TOGGLE_BBOX == wmId) {
        OnToggleBbox(hwnd);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT OnKeyDown(HWND hwnd, UINT msg, WPARAM key, LPARAM lParam)
{
    switch (key) {
    case VK_LEFT: case VK_PRIOR: case 'P':
        break;
    case VK_RIGHT: case VK_NEXT: case 'N':
        break;
    case VK_SPACE:
        break;
    case VK_F1:
        OnToggleBbox(hwnd);
        break;
    case VK_HOME:
        break;
    case VK_END:
        break;
    default:
        return DefWindowProc(hwnd, msg, key, lParam);
    }
    return 0;
}

static LRESULT CALLBACK WndProcFrame(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool seenWmPaint = false;

    if (gMainWnd) {
        bool wasHandled;
        LRESULT res = gMainWnd->evtMgr->OnMessage(msg, wParam, lParam, wasHandled);
        if (wasHandled)
            return res;
    }

    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_KEYDOWN:
            return OnKeyDown(hwnd, msg, wParam, lParam);

        case WM_COMMAND:
            OnCommand(hwnd, msg, wParam, lParam);
            break;
#if 0
        case WM_TIMER:
            OnTimer(hwnd, wParam);
            break;
#endif

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

static BOOL RegisterWinClass(HINSTANCE hInstance)
{
    WNDCLASSEX  wcex;

    FillWndClassEx(wcex, hInstance, FRAME_CLASS_NAME, WndProcFrame);
    // clear out CS_HREDRAW | CS_VREDRAW style so that resizing doesn't
    // cause the whole window redraw
    wcex.style = 0;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SUMATRAPDF));
    wcex.hbrBackground  = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    ATOM atom = RegisterClassEx(&wcex);
    return atom != NULL;
}

static BOOL InstanceInit(HINSTANCE hInstance, int nCmdShow)
{
    ghinst = hInstance;
    win::GetHwndDpi(NULL, &gUiDPIFactor);

    gHwndFrame = CreateWindow(
            FRAME_CLASS_NAME, L"Mui Test",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            dpiAdjust(WIN_DX), dpiAdjust(WIN_DY),
            NULL, NULL,
            ghinst, NULL);

    if (!gHwndFrame)
        return FALSE;
    gMainWnd = new HwndWrapper(gHwndFrame);
    //SetMenu(gHwndFrame, BuildMenu());
    ShowWindow(gHwndFrame, SW_SHOW);
    return TRUE;
}

static int RunMessageLoop()
{
    //HACCEL accTable = LoadAccelerators(ghinst, MAKEINTRESOURCE(IDC_SUMATRAPDF));
    MSG msg = { 0 };

    while (GetMessage(&msg, NULL, 0, 0)) {
        // dispatch the accelerator to the correct window
        //WindowInfo *win = FindWindowInfoByHwnd(msg.hwnd);
        //HWND accHwnd = win ? win->hwndFrame : msg.hwnd;
        //if (TranslateAccelerator(accHwnd, accTable, &msg))
        //    continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

#if 0
static void LoadWinDesc()
{
    ScopedMem<WCHAR> exePath(GetExePath());
    ScopedMem<WCHAR> exeDir(path::GetDir(exePath));

    str::Str<WCHAR> path;
    path.Append(exeDir);
    path.Append(L"\\..\\tools\\mui_test\\win_desc.txt");
    size_t sLen;
    char *s = file::ReadAll(path.Get(), &sLen);
    if (!s)
        return;
    TxtParser parser;
    parser.SetToParse(s, sLen);
    bool ok = ParseTxt(parser);
    if (!ok) {
        printf("failed to parse!");
    }
    free(s);
}
#endif

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int ret = 1;

#ifdef DEBUG
    // report memory leaks on DbgOut
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
    // ensure that C functions behave consistently under all OS locales
    // (use Win32 functions where localized input or output is desired)
    setlocale(LC_ALL, "C");

#if defined(DEBUG) && 0
    extern void SvgPath_UnitTests();
    SvgPath_UnitTests();
    extern void TrivialHtmlParser_UnitTests();
    TrivialHtmlParser_UnitTests();
    extern void HtmlPullParser_UnitTests();
    HtmlPullParser_UnitTests();
#endif
    ScopedCom com;
    InitAllCommonControls();
    ScopedGdiPlus gdi;

    mui::Initialize();

    //LoadWinDesc();

    if (!RegisterWinClass(hInstance))
        goto Exit;

    if (!InstanceInit(hInstance, nCmdShow))
        goto Exit;

    ret = RunMessageLoop();
    delete gMainWnd;

Exit:
    mui::Destroy();
    return ret;
}