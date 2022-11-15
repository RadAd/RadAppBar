#pragma once
#include <shellapi.h>

// https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shappbarmessage
// http://web.archive.org/web/20170704171718/https://www.microsoft.com/msj/archive/S274.aspx
// https://learn.microsoft.com/en-us/windows/win32/shell/application-desktop-toolbars

inline BOOL AppBarNew(HWND hWnd, UINT uCallbackMessage)
{
    APPBARDATA abd = { sizeof(APPBARDATA) };
    abd.hWnd = hWnd;
    abd.uCallbackMessage = uCallbackMessage;
    return static_cast<BOOL>(SHAppBarMessage(ABM_NEW, &abd));
}

inline void AppBarRemove(HWND hWnd)
{
    APPBARDATA abd = { sizeof(APPBARDATA) };
    abd.hWnd = hWnd;
    SHAppBarMessage(ABM_REMOVE, &abd);
}

inline void AppBarQueryPos(HWND hWnd, UINT uEdge, LPRECT prc)
{
    APPBARDATA abd = { sizeof(APPBARDATA) };
    abd.hWnd = hWnd;
    abd.uEdge = uEdge;
    abd.rc = *prc;
    SHAppBarMessage(ABM_QUERYPOS, &abd);
    *prc = abd.rc;
}

inline void AppBarSetPos(HWND hWnd, UINT uEdge, LPCRECT prc)
{
    APPBARDATA abd = { sizeof(APPBARDATA) };
    abd.hWnd = hWnd;
    abd.uEdge = uEdge;
    abd.rc = *prc;
    SHAppBarMessage(ABM_SETPOS, &abd);
}

inline BOOL AppBarSetAutoHideBar(HWND hWnd, UINT uEdge, BOOL bHide)
{
    APPBARDATA abd = { sizeof(APPBARDATA) };
    abd.hWnd = hWnd;
    abd.uEdge = uEdge;
    abd.lParam = bHide;
    return static_cast<BOOL>(SHAppBarMessage(ABM_SETAUTOHIDEBAR, &abd));
}

inline HWND AppBarGetAutoHideBar(HWND hWnd, UINT uEdge)
{
    APPBARDATA abd = { sizeof(APPBARDATA) };
    abd.hWnd = hWnd;
    abd.uEdge = uEdge;
    return static_cast<HWND>((VOID*) SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd));
}

inline void AppBarActivate(HWND hWnd)
{
    APPBARDATA abd = { sizeof(APPBARDATA) };
    abd.hWnd = hWnd;
    SHAppBarMessage(ABM_ACTIVATE, &abd);
}

inline void AppBarWindowPosChanged(HWND hWnd)
{
    APPBARDATA abd = { sizeof(APPBARDATA) };
    abd.hWnd = hWnd;
    SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);
}
