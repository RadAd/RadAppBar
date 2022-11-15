#pragma once

#include "Window.h"
#include "DoubleBufferedWindow.h"

struct Resources;

class WidgetWindow : public DoubleBufferedWindow
{
    typedef DoubleBufferedWindow Base;
    friend WindowManager<WidgetWindow>;
public:
    static ATOM Register() { return WindowManager<WidgetWindow>::Register(); }

protected:
    static void GetWndClass(WNDCLASS& wc);
    static void GetCreateWindow(CREATESTRUCT& cs);
    void HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

private:
    BOOL OnCreate(LPCREATESTRUCT lpCreateStruct, LRESULT* pResult);
    void OnSize(UINT state, int cx, int cy);
    BOOL OnEraseBkgnd(HDC hdc);
    void OnWidget(int id, LPARAM lParam);

protected:
    static LPCTSTR ClassName();

protected:
    SIZE GetSize() const;
    void FixSize();

    virtual SIZE CalcSize() = 0;
    virtual void SetEdge(UINT uEdge);

protected:
    UINT m_uEdge = 0; // ABE_TOP;
    const Resources* m_pResources = nullptr;
};
