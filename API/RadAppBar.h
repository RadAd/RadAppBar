#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define WC_WIDGET TEXT("Widget")

#define ABE_LEFT        0
#define ABE_TOP         1
#define ABE_RIGHT       2
#define ABE_BOTTOM      3

struct Resources
{
    COLORREF BackColor = RGB(255, 255, 255);
    COLORREF PanelColor = RGB(40, 40, 40);
    TCHAR FontFace[32] = TEXT("Segoe UI");
    LONG lFontHeight = 10;
    HFONT hFont = NULL;
    HFONT hFontHorz = NULL;
    COLORREF FontColor = RGB(0, 0, 0);
    COLORREF HighlightColor = RGB(0, 0, 0);
};

struct WidgetParams
{
    LPCTSTR Name;
    UINT Edge;
    Resources* Resources;
    HKEY hWidget;
};

typedef HWND CreateWidgetFn(HWND hWndParent, const WidgetParams* params);

#define WM_WIDGET (WM_USER+1838)

/* void Cls::OnWidget(int id, LPARAM lParam) */
#define HANDLEX_WM_WIDGET(wParam, lParam, fn) \
    ((fn)((int)(wParam), lParam), 0L)

enum RW_MESSAGE
{
    RW_SETEDGE,
};

inline void NotifyParent(HWND hWnd, int Notify)
{
    HWND hWndParent = GetParent(hWnd);
    SendMessage(hWndParent, WM_PARENTNOTIFY, MAKEWORD(Notify, GetDlgCtrlID(hWnd)), (LPARAM) hWnd);
}

#if 0
// Use to inform that this widget is about to be redrawn
// so the appbar needs to redraw the background
inline void InvalidateAppBar(HWND hWnd)
{
    RECT rc = {};
    GetWindowRect(hWnd, &rc);
    //ClientToScreen(*this, reinterpret_cast<POINT*>(&rc.left));
    //ClientToScreen(*this, reinterpret_cast<POINT*>(&rc.right));

    HWND hWndParent = GetParent(hWnd);
    ScreenToClient(hWndParent, reinterpret_cast<POINT*>(&rc.left));
    ScreenToClient(hWndParent, reinterpret_cast<POINT*>(&rc.right));
    InvalidateRect(hWndParent, &rc, TRUE);
}
#endif

#ifdef __cplusplus
}
#endif
