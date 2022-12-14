#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "TextWidgetWindow.h"
#include "..\API\RadAppBar.h"
#include "Windowxx.h"
#include "reg_utils.h"
#include "gdi_utils.h"
#include "memory_utils.h"

#include <algorithm>
#include <tchar.h>

TextWidgetWindow* TextWidgetWindow::Create(HWND hWndParent, const WidgetParams* params)
{
    return WindowManager<TextWidgetWindow>::Create(hWndParent, const_cast<WidgetParams*>(params));
}

SIZE TextWidgetWindow::CalcSize()
{
    SIZE sz = {};
    {
        auto hDC = MakeGetDC(*this);
        auto hOldFont = MakeSelectObject(-hDC, m_pResources->hFont);
        GetTextExtentPoint32(-hDC, m_strText, static_cast<int>(wcslen(m_strText)), &sz);
    }
    if (m_hIcon)
    {   // TODO Fix for ABE_LEFT, ABE_RIGHT
        sz.cx += GetSystemMetrics(SM_CXSMICON) + 2;
        sz.cy = std::max(sz.cy, (LONG) GetSystemMetrics(SM_CYSMICON));
    }
    if (m_uEdge == ABE_LEFT || m_uEdge == ABE_RIGHT)
        std::swap(sz.cx, sz.cy);
    return sz;
}

void TextWidgetWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    switch (uMsg)
    {
        HANDLE_NOT(WM_CREATE, Base::HandleMessage, OnCreate);
        HANDLE_DEF(Base::HandleMessage);
    }
}

void TextWidgetWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct, LRESULT* pResult)
{
    const WidgetParams* params = reinterpret_cast<WidgetParams*>(lpCreateStruct->lpCreateParams);

    TCHAR szIcon[MAX_PATH] = TEXT("");
    RegGetString(params->hWidget, TEXT("Icon"), szIcon, ARRAYSIZE(szIcon));

    HICON hIcon = (HICON) LoadImage(NULL, szIcon, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
    if (SetIcon(hIcon))
        FixSize();
}

void TextWidgetWindow::OnDraw(const PAINTSTRUCT* pps) const
{
    //Base::OnDraw(pps);

    HDC hDC = pps->hdc;

    RECT rc = { };
    GetClientRect(*this, &rc);

    auto hOldFont = MakeSelectObject(hDC, m_pResources->hFont);
    auto BkMode = MakeBkMode(hDC, TRANSPARENT);
    //auto BkColor = MakeBkColor(hDC, RGB(255, 0 , 0));
    auto TextColor = MakeTextColor(hDC, m_pResources->FontColor);

    POINT pt = { rc.left, rc.top };
    switch (m_uEdge)
    {
    case ABE_LEFT:
        pt.y = rc.bottom;
        break;
    case ABE_RIGHT:
        pt.x = rc.right;
        break;
    }

    if (m_hIcon != NULL)
    {
        // TODO Fix for ABE_LEFT, ABE_RIGHT
        const int w = 16;// GetSystemMetrics(SM_CXSMICON);
        const int h = 16;// GetSystemMetrics(SM_CYSMICON);
        DrawIconEx(hDC, pt.x, pt.y + (Height(rc) - h) / 2, m_hIcon, w, h, 0, NULL, DI_NORMAL);
        pt.x += h + 2;
    }

    TextOut(hDC, pt.x, pt.y, m_strText, static_cast<int>(wcslen(m_strText)));
}

bool TextWidgetWindow::SetText(LPCTSTR strText)
{
    if (_tcscmp(strText, m_strText) != 0)
    {
        _tcscpy_s(m_strText, strText);
        InvalidateRect(*this, nullptr, TRUE);
        return true;
    }
    else
        return false;
}

bool TextWidgetWindow::SetIcon(HICON hIcon)
{
    if (m_hIcon != hIcon)
    {
        m_hIcon = hIcon;
        InvalidateRect(*this, nullptr, TRUE);
        return true;
    }
    else
        return false;
}
