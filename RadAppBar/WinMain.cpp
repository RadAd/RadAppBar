#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#include <objbase.h>
#include <commctrl.h>

#include <Unknwn.h>
#include <Gdiplus.h>

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HINSTANCE g_hInstance = NULL;
HACCEL g_hAccelTable = NULL;

bool Init(_In_ const LPCTSTR lpCmdLine, _In_ const int nShowCmd);

int DoMessageLoop()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (g_hAccelTable == NULL || !TranslateAccelerator(msg.hwnd, g_hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}

int WINAPI _tWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nShowCmd)
{
    int ret = 0;
    g_hInstance = hInstance;
#ifdef _OBJBASE_H_  // from objbase.h
    if (FAILED(CoInitialize(nullptr)))
        return EXIT_FAILURE;
#endif
#ifdef _INC_COMMCTRL    // from commctrl.h
    InitCommonControls();
#endif
#ifdef _GDIPLUS_H   // from gdiplus.h
    ULONG_PTR token;
    Gdiplus::GdiplusStartupInput gi;
    if (Gdiplus::GdiplusStartup(&token, &gi, nullptr) != Gdiplus::Ok)
        return EXIT_FAILURE;
#endif

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (Init(lpCmdLine, nShowCmd))
        ret = DoMessageLoop();

#ifdef _GDIPLUS_H   // from gdiplus.h
    Gdiplus::GdiplusShutdown(token);
#endif
#ifdef _OBJBASE_H_
    CoUninitialize();
#endif

    return ret;
}
