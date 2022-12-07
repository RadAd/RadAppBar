#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "..\RABLib\Window.h"
#include "..\RABLib\DoubleBufferedWindow.h"
#include "..\RABLib\Windowxx.h"
#include "..\RABLib\WidgetWindow.h"
#include "..\RABLib\reg_utils.h"
#include "..\RABLib\gdi_utils.h"
#include "..\RABLib\memory_utils.h"
#include "..\RABLib\Logging.h"

#include "..\API\RadAppBar.h"

#include "AppBar.h"
#include "resource.h"

#include <tchar.h>
#include <string>
#include <vector>

#include <Unknwn.h>
#include <Gdiplus.h>

void GetRoundRectPath(Gdiplus::GraphicsPath* pPath, Gdiplus::Rect r, int dia)
{
    // diameter can't exceed width or height
    if (dia > r.Width)  dia = r.Width;
    if (dia > r.Height) dia = r.Height;

    // define a corner 
    Gdiplus::Rect Corner(r.X, r.Y, dia, dia);

    pPath->Reset();

    // top left
    pPath->AddArc(Corner, 180, 90);

#if 0
    // tweak needed for radius of 10 (dia of 20)
    if (dia == 20)
    {
        Corner.Width += 1;
        Corner.Height += 1;
        r.Width -= 1;
        r.Height -= 1;
    }
#endif

    // top right
    Corner.X += (r.Width - dia - 1);
    pPath->AddArc(Corner, 270, 90);

    // bottom right
    Corner.Y += (r.Height - dia - 1);
    pPath->AddArc(Corner, 0, 90);

    // bottom left
    Corner.X -= (r.Width - dia - 1);
    pPath->AddArc(Corner, 90, 90);

    // end path
    pPath->CloseFigure();
}

inline Gdiplus::Rect ToGdiRect(RECT r)
{
    return Gdiplus::Rect(r.left, r.top, Width(r), Height(r));
}

inline COLORREF Swap(COLORREF c)
{
    RGBQUAD& q = reinterpret_cast<RGBQUAD&>(c);
    BYTE t = q.rgbRed;
    q.rgbRed = q.rgbBlue;
    q.rgbBlue = t;
    return c;
}

COLORREF FixColor(COLORREF c)
{
    RGBQUAD& q = reinterpret_cast<RGBQUAD&>(c);
    if (q.rgbReserved == 0)
        q.rgbReserved = 254;
    return c;
}

void SetWindowBlur(HWND hWnd)
{
    const HINSTANCE hModule = GetModuleHandle(TEXT("user32.dll"));
    if (hModule)
    {
        typedef enum _ACCENT_STATE {
            ACCENT_DISABLED,
            ACCENT_ENABLE_GRADIENT,
            ACCENT_ENABLE_TRANSPARENTGRADIENT,
            ACCENT_ENABLE_BLURBEHIND,
            ACCENT_ENABLE_ACRYLICBLURBEHIND,
            ACCENT_INVALID_STATE
        } ACCENT_STATE;
        struct ACCENTPOLICY
        {
            ACCENT_STATE nAccentState;
            DWORD nFlags;
            DWORD nColor;
            DWORD nAnimationId;
        };
        typedef enum _WINDOWCOMPOSITIONATTRIB {
            WCA_UNDEFINED = 0,
            WCA_NCRENDERING_ENABLED = 1,
            WCA_NCRENDERING_POLICY = 2,
            WCA_TRANSITIONS_FORCEDISABLED = 3,
            WCA_ALLOW_NCPAINT = 4,
            WCA_CAPTION_BUTTON_BOUNDS = 5,
            WCA_NONCLIENT_RTL_LAYOUT = 6,
            WCA_FORCE_ICONIC_REPRESENTATION = 7,
            WCA_EXTENDED_FRAME_BOUNDS = 8,
            WCA_HAS_ICONIC_BITMAP = 9,
            WCA_THEME_ATTRIBUTES = 10,
            WCA_NCRENDERING_EXILED = 11,
            WCA_NCADORNMENTINFO = 12,
            WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
            WCA_VIDEO_OVERLAY_ACTIVE = 14,
            WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
            WCA_DISALLOW_PEEK = 16,
            WCA_CLOAK = 17,
            WCA_CLOAKED = 18,
            WCA_ACCENT_POLICY = 19,
            WCA_FREEZE_REPRESENTATION = 20,
            WCA_EVER_UNCLOAKED = 21,
            WCA_VISUAL_OWNER = 22,
            WCA_LAST = 23
        } WINDOWCOMPOSITIONATTRIB;
        struct WINCOMPATTRDATA
        {
            WINDOWCOMPOSITIONATTRIB nAttribute;
            PVOID pData;
            ULONG ulDataSize;
        };
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
        const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute) GetProcAddress(hModule, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute)
        {
            //ACCENTPOLICY policy = { ACCENT_ENABLE_ACRYLICBLURBEHIND, 0, 0x20FF0000, 0 };
            ACCENTPOLICY policy = { ACCENT_ENABLE_BLURBEHIND };
            WINCOMPATTRDATA data = { WCA_ACCENT_POLICY, &policy, sizeof(ACCENTPOLICY) };
            SetWindowCompositionAttribute(hWnd, &data);
            //DwmSetWindowAttribute(hWnd, WCA_ACCENT_POLICY, &policy, sizeof(ACCENTPOLICY));
        }
        //FreeLibrary(hModule);
    }
}

extern HINSTANCE g_hInstance;

inline LONG CalcFontHeight(HWND hWnd, LONG lHeight)
{
    HDC hDC = GetDC(hWnd);
    CHECK_RET(LogLevel::ERROR, hDC != NULL, lHeight);
    LONG lRet = -MulDiv(lHeight, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    ReleaseDC(hWnd, hDC);
    return lRet;
}

inline HFONT CreateFont(HWND hWnd, LPCTSTR pFaceName, LONG lHeight, LONG lAngle, LONG lWeight = FW_NORMAL)
{
    LOGFONT lf = {};
    _tcscpy_s(lf.lfFaceName, pFaceName);
    lf.lfHeight = CalcFontHeight(hWnd, lHeight);
    lf.lfWeight = lWeight;
    lf.lfEscapement = lAngle;
    //lf.lfOrientation = lAngle;
    return CreateFontIndirect(&lf);
}

#define APPNAME TEXT("RadAppBar")
#define WM_APPBAR (WM_USER+838)

/* void Cls::OnAppBar(int id, BOOL f) */
#define HANDLEX_WM_APPBAR(wParam, lParam, fn) \
    ((fn)((int)(wParam), (BOOL)(lParam)), 0L)

#define NUM_SECTIONS 3

struct AppBarCreate
{
    HKEYPtr hKeyWidgets;
    HKEYPtr hKeyBar;
};

class RootWindow : public DoubleBufferedWindow
{
    typedef DoubleBufferedWindow Base;
    friend WindowManager<RootWindow>;
public:
    static ATOM Register() { return WindowManager<RootWindow>::Register(); }
    static RootWindow* Create(const AppBarCreate& abc) { return WindowManager<RootWindow>::Create(const_cast<AppBarCreate*>(&abc)); }

protected:
    static void GetCreateWindow(CREATESTRUCT& cs);
    void HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

private:
    void OnCreate(LPCREATESTRUCT lpCreateStruct, LRESULT* pResult);
    void OnDestroy();
    BOOL OnEraseBkgnd(HDC hdc);
    void OnActivate(UINT state, HWND hwndActDeact, BOOL fMinimized);
    void OnWindowPosChanged(const LPWINDOWPOS lpwpos);
    void OnSize(UINT state, int cx, int cy);
    void OnSetFocus(HWND hwndOldFocus);
    void OnContextMenu(HWND hwndContext, UINT xPos, UINT yPos);
    void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    void OnParentNotify(UINT msg, HWND hwndChild, int idChild);
    void OnAppBar(int id, BOOL f);

    static LPCTSTR ClassName() { return TEXT("RadAppBar"); }

private:
    virtual void OnDraw(const PAINTSTRUCT* pps) const override;
    void PositionAppBar();
    void PositionWidgets();
    void CreateFonts();
    void SetEdge(UINT uEdge);

    UINT m_uEdge = ABE_TOP;
    Resources m_Default;
    RECT m_Margin = { 5, 5, 5, 5 };
    std::vector<HWND> m_Widgets[NUM_SECTIONS];
    RECT m_Panel[NUM_SECTIONS];

protected: // Helpers
    LRESULT SendMessage(_In_ UINT Msg, _Pre_maybenull_ _Post_valid_ WPARAM wParam = 0, _Pre_maybenull_ _Post_valid_ LPARAM lParam = 0) const { return ::SendMessage(*this, Msg, wParam, lParam); }
    BOOL MoveWindow(_In_ const RECT& rc, _In_ BOOL bRepaint) { return ::MoveWindow(*this, rc.left, rc.top, Width(rc), Height(rc), bRepaint); }
    BOOL SetWindowPos(_In_ const RECT& rc, _In_ UINT uFlags) { return ::SetWindowPos(*this, NULL, rc.left, rc.top, Width(rc), Height(rc), SWP_NOZORDER | uFlags); }
    BOOL SetWindowPos(_In_opt_ HWND hWndInsertAfter, _In_ UINT uFlags) { return ::SetWindowPos(*this, hWndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | uFlags); }
};

void RootWindow::GetCreateWindow(CREATESTRUCT& cs)
{
    Base::GetCreateWindow(cs);
    cs.lpszName = APPNAME;
    cs.style = WS_POPUP | WS_CLIPCHILDREN;
    cs.dwExStyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW; // | WS_EX_LAYERED;
}

void RootWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct, LRESULT* pResult)
{
    const AppBarCreate& abc = *reinterpret_cast<AppBarCreate*>(lpCreateStruct->lpCreateParams);

    m_uEdge = RegGetDWORD(-abc.hKeyBar, _T("Edge"), ABE_TOP);
    m_Default.BackColor = FixColor(RegGetDWORD(-abc.hKeyBar, _T("BackColor"), m_Default.BackColor));
    m_Default.PanelColor = FixColor(RegGetDWORD(-abc.hKeyBar, _T("PanelColor"), m_Default.PanelColor));
    m_Default.FontColor = FixColor(RegGetDWORD(-abc.hKeyBar, _T("FontColor"), m_Default.FontColor));
    m_Default.HighlightColor = FixColor(RegGetDWORD(-abc.hKeyBar, _T("HighlightColor"), m_Default.HighlightColor));
    RegGetString(-abc.hKeyBar, _T("FontFace"), m_Default.FontFace, ARRAYSIZE(m_Default.FontFace));
    m_Default.lFontHeight = RegGetDWORD(-abc.hKeyBar, _T("FontHeight"), m_Default.lFontHeight);
    CreateFonts();

    const LPCTSTR strSection[] = { _T("left"), _T("centre"), _T("right") };

    for (int i = 0; i < NUM_SECTIONS; ++i)
    {
        TCHAR strTestWidgets[MAX_PATH];
        CHECK(LogLevel::ERROR, RegGetString(-abc.hKeyBar, strSection[i], strTestWidgets, ARRAYSIZE(strTestWidgets)) == ERROR_SUCCESS);

        LPCTSTR strWidget = strTestWidgets;
        while (*strWidget != _T('\0'))
        {
            HKEYPtr hKeyThisWidget;
            if (SUCCEEDED(RegOpenKey(-abc.hKeyWidgets, strWidget, &hKeyThisWidget)))
            {
                const WidgetParams wp = { strWidget, m_uEdge, &m_Default, -hKeyThisWidget };

                TCHAR strModule[MAX_PATH] = TEXT("");
                CHECK(LogLevel::ERROR, RegGetString(-hKeyThisWidget, _T("Module"), strModule, ARRAYSIZE(strModule)) == ERROR_SUCCESS);

                char strEntry[100] = "";
                CHECK(LogLevel::ERROR, RegGetStringA(-hKeyThisWidget, "Entry", strEntry, ARRAYSIZE(strEntry)) == ERROR_SUCCESS);

                HMODULE hDLL = LoadLibrary(strModule);  // TODO Unload dll
                CHECK(LogLevel::ERROR, hDLL != NULL, strModule);
                if (hDLL != NULL)
                {
                    CreateWidgetFn* CreateWidget = reinterpret_cast<CreateWidgetFn*>(GetProcAddress(hDLL, strEntry));
                    CHECK(LogLevel::ERROR, CreateWidget != NULL);
                    if (CreateWidget != nullptr)
                    {
                        m_Widgets[i].push_back(CreateWidget(*this, &wp));
                    }
                }
            }

            strWidget += _tcslen(strWidget) + 1;
        }
    }

    AppBarNew(*this, WM_APPBAR);
    PositionAppBar();
    PositionWidgets();

    SetWindowBlur(*this);
}

void RootWindow::OnDestroy()
{
    DeleteFont(m_Default.hFont);
    m_Default.hFont = NULL;
    DeleteFont(m_Default.hFontHorz);
    m_Default.hFontHorz = NULL;

    AppBarRemove(*this);
    PostQuitMessage(0);
}

BOOL RootWindow::OnEraseBkgnd(HDC hdc)
{
    RECT rc;
    CHECKLE_RET(LogLevel::ERROR, GetClientRect(*this, &rc), FALSE);
#if 0
    auto oldColor = MakeDCBrushColor(hdc, m_Default.BackColor);
    FillRect(hdc, &rc, GetStockBrush(DC_BRUSH));
#else
    Gdiplus::Graphics g(hdc);
    Gdiplus::SolidBrush brush(Swap(m_Default.BackColor));
    g.FillRectangle(&brush, ToGdiRect(rc));
#endif
    return TRUE;
}

void RootWindow::OnActivate(UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    AppBarActivate(*this);
}

void RootWindow::OnWindowPosChanged(const LPWINDOWPOS lpwpos)
{
    AppBarWindowPosChanged(*this);
}

void RootWindow::OnSize(const UINT state, const int cx, const int cy)
{
}

void RootWindow::OnSetFocus(const HWND hwndOldFocus)
{
}

void RootWindow::OnContextMenu(HWND hwndContext, UINT xPos, UINT yPos)
{
    MenuPtr hMenu(LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_MENU1)));
    CHECKLE(LogLevel::ERROR, hMenu != NULL);
    UINT item = 0;
    switch (m_uEdge)
    {
    case ABE_TOP: item = ID_DOCK_TOP; break;
    case ABE_BOTTOM: item = ID_DOCK_BOTTOM; break;
    case ABE_LEFT: item = ID_DOCK_LEFT; break;
    case ABE_RIGHT: item = ID_DOCK_RIGHT; break;
    }
    //CheckMenuItem(hMenu, item, MF_CHECKED | MF_BYCOMMAND);
    CHECKLE(LogLevel::ERROR, CheckMenuRadioItem(-hMenu, ID_DOCK_TOP, ID_DOCK_RIGHT, item, MF_CHECKED | MF_BYCOMMAND));
    CHECKLE(LogLevel::ERROR, TrackPopupMenu(GetSubMenu(-hMenu, 0), 0, xPos, yPos, 0, *this, nullptr));
}

void RootWindow::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_POPUP_EXIT:
        SendMessage(WM_CLOSE);
        break;
    case ID_DOCK_TOP:
        SetEdge(ABE_TOP);
        break;
    case ID_DOCK_BOTTOM:
        SetEdge(ABE_BOTTOM);
        break;
    case ID_DOCK_LEFT:
        SetEdge(ABE_LEFT);
        break;
    case ID_DOCK_RIGHT:
        SetEdge(ABE_RIGHT);
        break;
    }
}

void RootWindow::OnParentNotify(UINT msg, HWND hwndChild, int idChild)
{
    switch (msg)
    {
    case WM_SIZE:
        //InvalidateRect(*this, nullptr, TRUE);
        PositionAppBar();
        PositionWidgets();  // TODO Somehow this is causing the child window to resize
        break;
    }
}

void RootWindow::OnAppBar(int id, BOOL f)
{
    switch (id)
    {
    case ABN_FULLSCREENAPP:
        CHECKLE(LogLevel::ERROR, SetWindowPos(f ? HWND_BOTTOM : HWND_TOPMOST, SWP_NOACTIVATE));
        break;

    case ABN_POSCHANGED:
        PositionAppBar();
        PositionWidgets();
        break;
    }
}

void RootWindow::HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam, LRESULT* pResult)
{
    switch (uMsg)
    {
        HANDLE_NOT(WM_CREATE, Base::HandleMessage, OnCreate);
        HANDLE_NOT(WM_DESTROY, Base::HandleMessage, OnDestroy);
        HANDLE_MSG(WM_ERASEBKGND, OnEraseBkgnd);
        HANDLE_NOT(WM_ACTIVATE, Base::HandleMessage, OnActivate);
        HANDLE_NOT(WM_WINDOWPOSCHANGED, Base::HandleMessage, OnWindowPosChanged);
        HANDLE_NOT(WM_SIZE, Base::HandleMessage, OnSize);
        HANDLE_NOT(WM_SETFOCUS, Base::HandleMessage, OnSetFocus);
        HANDLE_MSG(WM_CONTEXTMENU, OnContextMenu);
        HANDLE_NOT(WM_COMMAND, Base::HandleMessage, OnCommand);
        HANDLE_NOT(WM_PARENTNOTIFY, Base::HandleMessage, OnParentNotify);
        HANDLE_MSG(WM_APPBAR, OnAppBar);
        HANDLE_DEF(Base::HandleMessage);
    }
}

inline std::vector<RECT> GetRects(const std::vector<HWND>& Widgets)
{
    std::vector<RECT> rcs;
    rcs.reserve(Widgets.size());
    for (HWND hWndWidget : Widgets)
    {
        RECT rc = {};
        CHECKLE(LogLevel::ERROR, GetWindowRect(hWndWidget, &rc));
        HWND hWndParent = GetParent(hWndWidget);
        LPPOINT pt = (LPPOINT) &rc;
        CHECKLE(LogLevel::ERROR, ScreenToClient(hWndParent, &pt[0]));
        CHECKLE(LogLevel::ERROR, ScreenToClient(hWndParent, &pt[1]));
        rcs.push_back(rc);
    }
    return rcs;
}

void RootWindow::OnDraw(const PAINTSTRUCT* pps) const
{
#if 0
    HDC hDC = pps->hdc;

    auto DCPenColor = MakeDCPenColor(hDC, RGB(100, 100, 100));
    auto DCBrushColor = MakeDCBrushColor(hDC, m_Default.PanelColor);
    auto hOldPen = MakeSelectObject(hDC, GetStockObject(DC_PEN));
    auto hOldBrush = MakeSelectObject(hDC, GetStockObject(DC_BRUSH));
#else
    Gdiplus::Graphics g(pps->hdc);
    Gdiplus::Pen pen(Gdiplus::Color(255, 100, 100, 100));
    pen.SetAlignment(Gdiplus::PenAlignmentCenter);
    Gdiplus::SolidBrush brush(Swap(m_Default.PanelColor));
#endif

    for (int i = 0; i < NUM_SECTIONS; ++i)
    {
        const int RoundSize = std::min(Width(m_Panel[i]), Height(m_Panel[i]));
#if 0
        RoundRect(hDC, m_Panel[i], RoundSize, RoundSize);
#else
        Gdiplus::GraphicsPath path;
        GetRoundRectPath(&path, ToGdiRect(m_Panel[i]), RoundSize);
        g.FillPath(&brush, &path);
        g.DrawPath(&pen, &path);
#endif

        const std::vector<RECT> rcs = GetRects(m_Widgets[i]);
        if (!rcs.empty())
        {
            auto it = rcs.begin();
            auto e = it->right;
            ++it;
            for (; it != rcs.end(); ++it)
            {
                const int x = (e + it->left) / 2;
#if 0
                MoveToEx(hDC, x, it->top, nullptr);
                LineTo(hDC, x, it->bottom);
#else
                g.DrawLine(&pen, Gdiplus::Point(x, it->top), Gdiplus::Point(x, it->bottom));
#endif
                e = it->right;
            }
        }
    }
}

void RootWindow::PositionAppBar()
{
    SIZE sz = {};
    for (const auto& ws : m_Widgets)
    {
        for (HWND hWndWidget : ws)
        {
            RECT rc = {};
            CHECKLE(LogLevel::ERROR, GetWindowRect(hWndWidget, &rc));
            const SIZE szWidget = Size(rc);

            sz.cx = std::max(sz.cx, szWidget.cx);
            sz.cy = std::max(sz.cy, szWidget.cy);
        }
    }

    const LONG HorzBarHeight = sz.cy + m_Margin.top + m_Margin.bottom;
    const LONG VertBarWidth = sz.cx + m_Margin.left + m_Margin.right;

    RECT rc = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    AppBarQueryPos(*this, m_uEdge, &rc);
    switch (m_uEdge)
    {
    case ABE_TOP: rc.bottom = rc.top + HorzBarHeight; break;
    case ABE_BOTTOM: rc.top = rc.bottom - HorzBarHeight; break;
    case ABE_LEFT: rc.right = rc.left + VertBarWidth; break;
    case ABE_RIGHT: rc.left = rc.right - VertBarWidth; break;
    }
    AppBarSetPos(*this, m_uEdge, &rc);
    //MoveWindow(rc, TRUE);
    CHECKLE(LogLevel::ERROR, SetWindowPos(rc, SWP_NOACTIVATE));
}

void RootWindow::PositionWidgets() 
{
    const SIZE padding = { 10, 10 };

    const std::vector<RECT> rcs[NUM_SECTIONS] = { GetRects(m_Widgets[0]), GetRects(m_Widgets[1]), GetRects(m_Widgets[2]) };

    SIZE total[NUM_SECTIONS] = { padding, padding, padding };
    for (int i = 0; i < NUM_SECTIONS; ++i)
        for (const RECT& rc : rcs[i])
        {
            total[i].cx += Width(rc) + padding.cx;
            total[i].cy += Height(rc) + padding.cy;
        }

    RECT rcClient;
    CHECKLE(LogLevel::ERROR, GetClientRect(*this, &rcClient));

    switch (m_uEdge)
    {
    case ABE_TOP:
    case ABE_BOTTOM:
    {
        const POINT ptStart[NUM_SECTIONS] = {
            { rcClient.left + m_Margin.left, rcClient.top + m_Margin.top },
            { rcClient.left + (rcClient.left + rcClient.right - total[1].cx) / 2, rcClient.top + m_Margin.top  },
            { rcClient.right - m_Margin.right - total[2].cx, rcClient.top + m_Margin.top  }
        };
        for (int i = 0; i < NUM_SECTIONS; ++i)
        {
            auto rcWidgetIt = rcs[i].begin();
            POINT pt = ptStart[i];
            m_Panel[i].top = pt.y;  // TODO For ABE_LEFT and ABE_RIGHT also
            m_Panel[i].left = pt.x;
            m_Panel[i].bottom = m_Panel[i].top;
            pt.x += padding.cx;
            for (HWND hWndWidget : m_Widgets[i])
            {
                const RECT& rcWidget = *rcWidgetIt++;

                if (pt.x != rcWidget.left || pt.y != rcWidget.top)
                    CHECKLE(LogLevel::ERROR, ::SetWindowPos(hWndWidget, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));

                pt.x += Width(rcWidget) + padding.cx;
                m_Panel[i].bottom = std::max(m_Panel[i].bottom, m_Panel[i].top + Height(rcWidget));
            }
            m_Panel[i].right = pt.x;
            m_Panel[i].left -= 2;
            InflateRect(&m_Panel[i], 2, 2);
        }
    }
    break;

    case ABE_LEFT:
    {
        const POINT ptStart[NUM_SECTIONS] = {
            { rcClient.left + m_Margin.left, rcClient.bottom - m_Margin.bottom },
            { rcClient.left + m_Margin.left, rcClient.bottom - (rcClient.top + rcClient.bottom - total[1].cy) / 2 },
            { rcClient.left + m_Margin.left, rcClient.top + m_Margin.top + total[2].cy }
        };
        for (int i = 0; i < NUM_SECTIONS; ++i)
        {
            auto rcWidgetIt = rcs[i].begin();
            POINT pt = ptStart[i];
            for (HWND hWndWidget : m_Widgets[i])
            {
                const RECT& rcWidget = *rcWidgetIt++;

                if (pt.x != rcWidget.left || (pt.y - Height(rcWidget)) != rcWidget.top)
                    CHECKLE(LogLevel::ERROR, ::SetWindowPos(hWndWidget, NULL, pt.x, pt.y - Height(rcWidget), 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));

                pt.y -= Height(rcWidget) + padding.cy;
            }
        }
    }
    break;
    case ABE_RIGHT:
    {
        const POINT ptStart[NUM_SECTIONS] = {
            { rcClient.left + m_Margin.left, rcClient.top + m_Margin.top },
            { rcClient.left + m_Margin.left, rcClient.top + (rcClient.top + rcClient.bottom - total[1].cy) / 2 },
            { rcClient.left + m_Margin.left, rcClient.bottom - m_Margin.bottom - total[2].cy }
        };
        for (int i = 0; i < NUM_SECTIONS; ++i)
        {
            auto rcWidgetIt = rcs[i].begin();
            POINT pt = ptStart[i];
            for (HWND hWndWidget : m_Widgets[i])
            {
                const RECT& rcWidget = *rcWidgetIt++;

                if (pt.x != rcWidget.left || pt.y != rcWidget.top)
                    CHECKLE(LogLevel::ERROR, ::SetWindowPos(hWndWidget, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE));

                pt.y += Height(rcWidget) + padding.cy;
            }
        }
    }
    break;
    }

    InvalidateRect(*this, nullptr, TRUE);
}

void RootWindow::CreateFonts()
{
    DeleteFont(m_Default.hFont);
    const int Angles[] = { 900, 0, 2700, 0 };
    CHECK(LogLevel::ERROR, m_Default.hFont = CreateFont(*this, m_Default.FontFace, m_Default.lFontHeight, Angles[m_uEdge]));
    if (m_Default.hFontHorz == NULL)
        CHECK(LogLevel::ERROR, m_Default.hFontHorz = CreateFont(*this, m_Default.FontFace, m_Default.lFontHeight, 0));
}

void RootWindow::SetEdge(UINT uEdge)
{
    m_uEdge = uEdge;
    CreateFonts();

    for (const auto& ws : m_Widgets)
        for (HWND hWndWidget : ws)
            ::SendMessage(hWndWidget, WM_WIDGET, RW_SETEDGE, m_uEdge);

    PositionAppBar();
    PositionWidgets();
}

bool Init(_In_ const LPCTSTR lpCmdLine, _In_ const int nShowCmd)
{
    InitLog(NULL, APPNAME);

    CHECKLE_RET(LogLevel::ERROR, RootWindow::Register() != NULL, false);
    CHECKLE_RET(LogLevel::ERROR, WidgetWindow::Register() != NULL, false);

    {
        AppBarCreate abc;
        HKEYPtr hKeyMain;
        CHECKHR_RET(LogLevel::ERROR, RegOpenKey(HKEY_CURRENT_USER, _T(R"-(SOFTWARE\RadSoft\RadAppBar)-"), &hKeyMain), false);
        CHECKHR_RET(LogLevel::ERROR, RegOpenKey(-hKeyMain, _T("Widgets"), &abc.hKeyWidgets), false);
        HKEYPtr hKeyBars;
        CHECKHR_RET(LogLevel::ERROR, RegOpenKey(-hKeyMain, _T("Bars"), &hKeyBars), false);

        TCHAR BarName[1024];
        DWORD cbBarName = 0;
        for (int i = 0; cbBarName = ARRAYSIZE(BarName), RegEnumKeyEx(-hKeyBars, i, BarName, &cbBarName, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS; ++i)
        {
            CHECKHR_RET(LogLevel::ERROR, RegOpenKey(-hKeyBars, BarName, &abc.hKeyBar), false);
            RootWindow* prw = RootWindow::Create(abc);
            CHECK_RET(LogLevel::ERROR, prw != nullptr, false, TEXT("Error creating root window"));
            InitLog(*prw, APPNAME); // TODO Might be better to have a hidden "main" window
            ShowWindow(*prw, nShowCmd);
        }
    }

    return true;
}
