#pragma once

#include "Window.h"

typedef struct tagPAINTSTRUCTDB : PAINTSTRUCT {
    HDC hdcwnd;
    HBITMAP hbmp;
    HBITMAP hbmpold;
} PAINTSTRUCTDB, * PPAINTSTRUCTDB, * NPPAINTSTRUCTDB, * LPPAINTSTRUCTDB;

inline HDC WINAPI BeginPaintDB(_In_ HWND hWnd, _Out_ LPPAINTSTRUCTDB lpPaint)
{
    // Suppress WM_ERASEBKGND
    lpPaint->fErase = GetUpdateRect(hWnd, &lpPaint->rcPaint, FALSE);
    //lpPaint->hdcwnd = GetDC(hWnd);
    LONG style = (LONG) GetWindowLongPtr(hWnd, GWL_STYLE);
    HRGN hRgn = CreateRectRgnIndirect(&lpPaint->rcPaint);
    DWORD flags = DCX_CACHE;
    if (style & WS_CLIPCHILDREN)
        flags |= DCX_CLIPCHILDREN;
    if (style & WS_CLIPSIBLINGS)
        flags |= DCX_CLIPSIBLINGS;
    lpPaint->hdcwnd = GetDCEx(hWnd, hRgn, flags);
    DeleteObject(hRgn);
    lpPaint->hdc = CreateCompatibleDC(lpPaint->hdcwnd);
    lpPaint->hbmp = CreateCompatibleBitmap(lpPaint->hdcwnd, lpPaint->rcPaint.right - lpPaint->rcPaint.left, lpPaint->rcPaint.bottom - lpPaint->rcPaint.top);
    lpPaint->hbmpold = (HBITMAP) SelectObject(lpPaint->hdc, lpPaint->hbmp);
    SetViewportOrgEx(lpPaint->hdc, lpPaint->rcPaint.left, lpPaint->rcPaint.top, nullptr);
    lpPaint->fErase |= SendMessage(hWnd, WM_ERASEBKGND, (WPARAM) lpPaint->hdc, 0);

    return lpPaint->hdc;
}

inline BOOL WINAPI EndPaintDB(_In_ HWND hWnd, _In_ CONST PAINTSTRUCTDB* lpPaint)
{
    BitBlt(lpPaint->hdcwnd, lpPaint->rcPaint.left, lpPaint->rcPaint.top, lpPaint->rcPaint.right - lpPaint->rcPaint.left, lpPaint->rcPaint.bottom - lpPaint->rcPaint.top, lpPaint->hdc, lpPaint->rcPaint.left, lpPaint->rcPaint.top, SRCCOPY);

    SelectObject(lpPaint->hdc, lpPaint->hbmpold);
    DeleteObject(lpPaint->hbmp);
    DeleteDC(lpPaint->hdc);
    ReleaseDC(hWnd, lpPaint->hdcwnd);
    ValidateRect(hWnd, nullptr);

    return TRUE;
}

class DoubleBufferedWindow : public Window
{
protected:
    void HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override
    {
        switch (uMsg)
        {
        case WM_PAINT:
            OnPaint();
            *pResult = 0;
            break;
        default:
            Window::HandleMessage(uMsg, wParam, lParam, pResult);
            break;
        }
    }

protected:
    void OnPaint()
    {
#if 0
        PAINTSTRUCT ps;
        BeginPaint(*this, &ps);
        OnDraw(&ps);
        EndPaint(*this, &ps);
#else
        PAINTSTRUCTDB ps;
        BeginPaintDB(*this, &ps);
        OnDraw(&ps);
        EndPaintDB(*this, &ps);
#endif
    }
};
