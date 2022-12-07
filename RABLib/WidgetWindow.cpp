#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "WidgetWindow.h"
#include "Windowxx.h"
#include "..\API\RadAppBar.h"
#include "gdi_utils.h"
//#include "memory_utils.h"

ATOM WidgetWindowRegister() { return WidgetWindow::Register(); }

LPCTSTR WidgetWindow::ClassName() { return WC_WIDGET; }

void WidgetWindow::GetWndClass(WNDCLASS& wc)
{
    Base::GetWndClass(wc);
    wc.hIcon = NULL;
    wc.hbrBackground = NULL;
}

void WidgetWindow::GetCreateWindow(CREATESTRUCT& cs)
{
    Base::GetCreateWindow(cs);
    cs.style = WS_CHILD | WS_VISIBLE;
    //cs.dwExStyle |= WS_EX_TRANSPARENT;
}

BOOL WidgetWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct, LRESULT* pResult)
{
    const WidgetParams* params = reinterpret_cast<WidgetParams*>(lpCreateStruct->lpCreateParams);
    m_pResources = params->Resources;
    SetEdge(params->Edge);
    return TRUE;
}

void WidgetWindow::OnSize(UINT state, int cx, int cy)
{
    NotifyParent(*this, WM_SIZE);
}

BOOL WidgetWindow::OnEraseBkgnd(HDC hdc)
{
    auto DCBrushColor = MakeDCBrushColor(hdc, m_pResources->PanelBackColor);
    RECT rc;
    GetClientRect(*this, &rc);
    FillRect(hdc, &rc, GetStockBrush(DC_BRUSH));
    return TRUE;
}

void WidgetWindow::OnWidget(int id, LPARAM lParam)
{
    switch (id)
    {
    case RW_SETEDGE:
        SetEdge(static_cast<UINT>(lParam));
        break;
    }
}

void WidgetWindow::SetEdge(UINT uEdge)
{
    m_uEdge = uEdge;
    FixSize();
    InvalidateRect(*this, nullptr, TRUE);
}

void WidgetWindow::HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam, LRESULT* pResult)
{
    switch (uMsg)
    {
        HANDLE_NOT(WM_CREATE, Base::HandleMessage, OnCreate);
        HANDLE_MSG(WM_ERASEBKGND, OnEraseBkgnd);
        HANDLE_MSG(WM_WIDGET, OnWidget);
        HANDLE_NOT(WM_SIZE, Base::HandleMessage, OnSize);
        HANDLE_DEF(Base::HandleMessage);
    }
}

void WidgetWindow::OnPostDraw(const PAINTSTRUCT* pps) const
{
    if (!m_bHasAlpha)
        PixelBlt(pps->hdc, pps->rcPaint.left, pps->rcPaint.top, pps->rcPaint.right - pps->rcPaint.left, pps->rcPaint.bottom - pps->rcPaint.top, ARGB(255, 0, 0, 0), SRCPAINT);
}

SIZE WidgetWindow::GetSize() const
{
    RECT rc;
    GetWindowRect(*this, &rc);
    return Size(rc);
}

void WidgetWindow::FixSize()
{
    const SIZE sz = CalcSize();
    if (sz != GetSize())
        SetWindowPos(*this, NULL, 0, 0, sz.cx, sz.cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}
