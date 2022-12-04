#define STRICT
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "..\RABLib\WidgetWindow.h"
#include "..\RABLib\TextWidgetWindow.h"
#include "..\RABLib\Windowxx.h"
#include "..\RABLib\gdi_utils.h"
#include "..\RABLib\memory_utils.h"

#include "..\API\RadAppBar.h"
#include "Win10Desktops.h"

#include <tchar.h>
#include <algorithm>
#include <atlbase.h>
#include <winstring.h>

#include "ComUtils.h"
#include "VDNotification.h"

#define WM_UPDATE (WM_USER + 151)

/* void Cls::OnMouseLeave() */
#define HANDLEX_WM_MOUSELEAVE(wParam, lParam, fn) \
    ((fn)(), 0L)

class VDNotification
{
public:
    void Register(IServiceProvider* pServiceProvider, IVDNotification* notify)
    {
        pServiceProvider->QueryService(CLSID_VirtualNotificationService, &m_pDesktopNotificationService);
        RegisterForNotifications(notify);
    }

    ~VDNotification()
    {
        UnregisterForNotifications();
    }

private:
    void RegisterForNotifications(IVDNotification* notify)
    {
        m_pNotify = new VirtualDesktopNotification(notify);
        if (m_idVirtualDesktopNotification == 0 && m_pDesktopNotificationService)
        {
            m_pDesktopNotificationService->Register(m_pNotify, &m_idVirtualDesktopNotification);
        }
    }

    void UnregisterForNotifications()
    {
        if (m_idVirtualDesktopNotification != 0 && m_pDesktopNotificationService)
        {
            m_pDesktopNotificationService->Unregister(m_idVirtualDesktopNotification);
            m_idVirtualDesktopNotification = 0;
        }
    }

private:
    CComPtr<Win10::IVirtualDesktopNotificationService> m_pDesktopNotificationService;

    DWORD m_idVirtualDesktopNotification = 0;
    CComPtr<Win10::IVirtualDesktopNotification> m_pNotify;
};

class DesktopsWidgetWindow : public WidgetWindow, public IVDNotification
{
    typedef WidgetWindow Base;
    friend WindowManager<DesktopsWidgetWindow>;
public:
    static WidgetWindow* Create(HWND hWndParent, const WidgetParams* params)
    {
        return WindowManager<DesktopsWidgetWindow>::Create(hWndParent, params->Name, const_cast<WidgetParams*>(params));
    }

    DesktopsWidgetWindow()
    {
        m_pServiceProvider.CoCreateInstance(CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER);

        m_pServiceProvider->QueryService(CLSID_VirtualDesktopManagerInternal, &m_pDesktopManagerInternal);

        m_notifications.Register(m_pServiceProvider, this);
    }

protected:
    void HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override
    {
        switch (uMsg)
        {
        case WM_UPDATE:
            if (wParam)
                FixSize();
            InvalidateRect(*this, nullptr, TRUE);
            *pResult = 0;
            break;
            HANDLE_NOT(WM_MOUSEMOVE, Base::HandleMessage, OnMouseMove);
            HANDLE_NOT(WM_MOUSELEAVE, Base::HandleMessage, OnMouseLeave);
            HANDLE_NOT(WM_LBUTTONDOWN, Base::HandleMessage, OnLButtonDown);
            HANDLE_NOT(WM_LBUTTONUP, Base::HandleMessage, OnLButtonUp);
            HANDLE_DEF(Base::HandleMessage);
        }
    }

protected:
    virtual SIZE CalcSize() override
    {
        UINT uCount = 0;
        m_pDesktopManagerInternal->GetCount(&uCount);

        SIZE sz = { };
        {
            auto hDC = MakeGetDC(*this);
            auto hOldFont = MakeSelectObject(-hDC, m_pResources->hFontHorz);
            GetTextExtentPoint32(-hDC, TEXT("9"), 1, &sz);
        }
        //sz.cx *= uCount * 10;
        switch (m_uEdge)
        {
        case ABE_TOP:
        case ABE_BOTTOM:
            sz.cx = uCount * (sz.cy + 2);
            break;
        case ABE_LEFT:
        case ABE_RIGHT:
            std::swap(sz.cx, sz.cy);
            sz.cy = uCount * (sz.cx + 2);
            break;
        }
        return sz;
    }

    virtual void OnDraw(const PAINTSTRUCT* pps) const override
    {
        HDC hDC = pps->hdc;

        RECT rc = { };
        GetClientRect(*this, &rc);

        //auto DCPenColor = MakeDCPenColor(hDC, m_pResources->FontColor);
        auto DCPenColor = MakeDCPenColor(hDC, m_pResources->BackColor);
        auto DCBrushColor = MakeDCBrushColor(hDC, m_pResources->BackColor);
        auto hOldFont = MakeSelectObject(hDC, m_pResources->hFontHorz);
        auto hOldPen = MakeSelectObject(hDC, GetStockObject(DC_PEN));
        auto hOldBrush = MakeSelectObject(hDC, GetStockObject(DC_BRUSH));
        auto BkMode = MakeBkMode(hDC, TRANSPARENT);
        //auto BkColor = MakeBkColor(hDC, RGB(255, 0 , 0));
        auto BkColor = MakeTextColor(hDC, m_pResources->FontColor);

        switch (m_uEdge)
        {
        case ABE_TOP:
        case ABE_BOTTOM:
            rc.right = rc.left + Height(rc);
            break;
        case ABE_LEFT:
            rc.top = rc.bottom - Width(rc);
            break;
        case ABE_RIGHT:
            rc.bottom = rc.top + Width(rc);
            break;
        }

        CComPtr<Win10::IVirtualDesktop> pCurrentDesktop;
        m_pDesktopManagerInternal->GetCurrentDesktop(&pCurrentDesktop);

        CComPtr<IObjectArray> pDesktopArray;
        m_pDesktopManagerInternal->GetDesktops(&pDesktopArray);

        int dn = 1;
        for (CComPtr<Win10::IVirtualDesktop2> pDesktop : ObjectArrayRange<Win10::IVirtualDesktop2>(pDesktopArray))
        {
            TCHAR buf[100];
            int len = _stprintf_s(buf, TEXT("%d"), dn);

            SetDCPenColor(hDC, pDesktop.IsEqualObject(m_pHover) && (!m_pButton || pDesktop.IsEqualObject(m_pButton)) ? RGB(64, 64, 64) : m_pResources->BackColor);
            //SetDCBrushColor(hDC, pDesktop.IsEqualObject(pCurrentDesktop) ? RGB(128, 128, 128) : (pDesktop.IsEqualObject(m_pHover) ? RGB(64, 64, 256) : m_pResources->BackColor));
            SetDCBrushColor(hDC, pDesktop.IsEqualObject(m_pHover) ? RGB(64, 64, 64) : m_pResources->BackColor);

            Rectangle(hDC, rc);

            if (pDesktop.IsEqualObject(pCurrentDesktop))
            {
                SetDCPenColor(hDC, m_pResources->HighlightColor);
                SetDCBrushColor(hDC, m_pResources->HighlightColor);
                RECT rcHighlight = rc;
                switch (m_uEdge)
                {
                case ABE_TOP:
                case ABE_BOTTOM:
                    rcHighlight.top = rcHighlight.bottom - 3;
                    break;
                case ABE_LEFT:
                    rcHighlight.left = rcHighlight.right - 2;
                    break;
                case ABE_RIGHT:
                    rcHighlight.right = rcHighlight.left + 2;
                    break;
                }
                Rectangle(hDC, rcHighlight);
            }
            //TextOut(hDC, rc.left, rc.top, buf, len);
            DrawText(hDC, buf, len, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            switch (m_uEdge)
            {
            case ABE_TOP:
            case ABE_BOTTOM:
                OffsetRect(&rc, Width(rc) + 2, 0);
                break;
            case ABE_LEFT:
                OffsetRect(&rc, 0, -(Height(rc) + 2));
                break;
            case ABE_RIGHT:
                OffsetRect(&rc, 0, Height(rc) + 2);
                break;
            }

            ++dn;
        }
    }

    void OnMouseMove(int x, int y, UINT keyFlags)
    {
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
        tme.hwndTrack = *this;
        tme.dwFlags = TME_LEAVE;
        TrackMouseEvent(&tme);

        CComPtr<Win10::IVirtualDesktop> pNewHover = GetDesktop(x, y);
        if (!pNewHover.IsEqualObject(m_pHover))
        {
            m_pHover = pNewHover;
            InvalidateRect(*this, nullptr, TRUE);
        }
    }

    void OnMouseLeave()
    {
        m_pHover.Release();
        InvalidateRect(*this, nullptr, TRUE);
    }

    void OnLButtonDown(int x, int y, UINT keyFlags)
    {
        m_pButton = GetDesktop(x, y);
        if (m_pButton)
            SetCapture(*this);
    }

    void OnLButtonUp(int x, int y, UINT keyFlags)
    {
        if (m_pButton)
        {
            ReleaseCapture();

            if (m_pButton == GetDesktop(x, y))
                m_pDesktopManagerInternal->SwitchDesktop(m_pButton);

            m_pButton.Release();
            m_pHover.Release();
        }
    }

protected:
    CComPtr<Win10::IVirtualDesktop> GetDesktop(int x, int y)
    {
        RECT rc = { };
        GetClientRect(*this, &rc);

        if (PtInRect(&rc, POINT({ x, y })))
        {
            int dn;
            switch (m_uEdge)
            {
            case ABE_TOP:
            case ABE_BOTTOM:
            {
                const LONG width = Height(rc) + 2;
                dn = (x / width) + 1;
            }
            break;
            case ABE_LEFT:
            {
                const LONG height = Width(rc) + 2;
                dn = ((rc.bottom - y) / height) + 1;
            }
            break;
            case ABE_RIGHT:
            {
                const LONG height = Width(rc) + 2;
                dn = (y / height) + 1;
            }
            break;
            default:
                assert(false);
                dn = 0;
                break;
            }

            return GetDesktop(dn);
        }
        else
            return {};
    }

    CComPtr<Win10::IVirtualDesktop> GetDesktop(int dn)
    {
        CComPtr<IObjectArray> pDesktopArray;
        m_pDesktopManagerInternal->GetDesktops(&pDesktopArray);
        CComPtr<Win10::IVirtualDesktop> pDesktop;
        pDesktopArray->GetAt(dn - 1, IID_PPV_ARGS(&pDesktop));
        return pDesktop;
    }

private: // IVDNotification Win10::IVirtualDesktopNotification
    virtual void VirtualDesktopCreated(Win10::IVirtualDesktop* pDesktop) override
    {
        SendMessage(*this, WM_UPDATE, TRUE, 0);
    }

    virtual void VirtualDesktopDestroyed(Win10::IVirtualDesktop* pDesktopDestroyed, Win10::IVirtualDesktop* pDesktopFallback) override
    {
        SendMessage(*this, WM_UPDATE, TRUE, 0);
    }

    virtual void CurrentVirtualDesktopChanged(Win10::IVirtualDesktop* pDesktopOld, Win10::IVirtualDesktop* pDesktopNew) override
    {
        SendMessage(*this, WM_UPDATE, FALSE, 0);
    }

    virtual void VirtualDesktopNameChanged(Win10::IVirtualDesktop* pDesktop, HSTRING name) override
    {
        SendMessage(*this, WM_UPDATE, FALSE, 0);
    }

private: // IVDNotification Win11::IVirtualDesktopNotification
    virtual void VirtualDesktopCreated(Win11::IVirtualDesktop* pDesktop) override
    {
        SendMessage(*this, WM_UPDATE, TRUE, 0);
    }

    virtual void VirtualDesktopDestroyed(Win11::IVirtualDesktop* pDesktopDestroyed, Win11::IVirtualDesktop* pDesktopFallback) override
    {
        SendMessage(*this, WM_UPDATE, TRUE, 0);
    }

    virtual void VirtualDesktopMoved(Win11::IVirtualDesktop* pDesktop, int64_t oldIndex, int64_t newIndex) override
    {
        SendMessage(*this, WM_UPDATE, FALSE, 0);
    }

    virtual void VirtualDesktopNameChanged(Win11::IVirtualDesktop* pDesktop, HSTRING name) override
    {
        SendMessage(*this, WM_UPDATE, FALSE, 0);
    }

    virtual void CurrentVirtualDesktopChanged(Win11::IVirtualDesktop* pDesktopOld, Win11::IVirtualDesktop* pDesktopNew) override
    {
        SendMessage(*this, WM_UPDATE, FALSE, 0);
    }

private:
    CComPtr<IServiceProvider> m_pServiceProvider;
    CComPtr<Win10::IVirtualDesktopManagerInternal> m_pDesktopManagerInternal;

    VDNotification m_notifications;

    CComPtr<Win10::IVirtualDesktop> m_pHover;
    CComPtr<Win10::IVirtualDesktop> m_pButton;
};

class DesktopNameWidgetWindow : public TextWidgetWindow, public IVDNotification
{
    typedef TextWidgetWindow Base;
    friend WindowManager<DesktopNameWidgetWindow>;
public:
    static WidgetWindow* Create(HWND hWndParent, const WidgetParams* params)
    {
        return WindowManager<DesktopNameWidgetWindow>::Create(hWndParent, params->Name, const_cast<WidgetParams*>(params));
    }

protected:
    DesktopNameWidgetWindow()
    {
        m_pServiceProvider.CoCreateInstance(CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER);

        m_pServiceProvider->QueryService(CLSID_VirtualDesktopManagerInternal, &m_pDesktopManagerInternal);

        m_notifications.Register(m_pServiceProvider, this);

        CComPtr<Win10::IVirtualDesktop> pCurrentDesktop;
        m_pDesktopManagerInternal->GetCurrentDesktop(&pCurrentDesktop);

        Update(pCurrentDesktop);
    }

    bool Update(Win10::IVirtualDesktop* pDesktop)
    {
        CComQIPtr<Win10::IVirtualDesktop2> pDesktop2 = pDesktop;

        HSTRING hName = NULL;
        pDesktop2->GetName(&hName);

        if (hName != nullptr)
            return SetText(WindowsGetStringRawBuffer(hName, nullptr));
        else
        {
            TCHAR strText[100];
            _stprintf_s(strText, TEXT("Desktop %d"), 0);    // TODO Get desktop number
            return SetText(strText);
        }
    }

    void HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override
    {
        switch (uMsg)
        {
        case WM_UPDATE:
            if (Update(reinterpret_cast<Win10::IVirtualDesktop*>(lParam)))
                FixSize();
            *pResult = 0;
            break;
        default:
            Base::HandleMessage(uMsg, wParam, lParam, pResult);
            break;
        }
    }

private: // IVDNotification Win10::IVirtualDesktopNotification
    virtual void VirtualDesktopCreated(Win10::IVirtualDesktop* pDesktop) override
    {
    }

    virtual void VirtualDesktopDestroyed(Win10::IVirtualDesktop* pDesktopDestroyed, Win10::IVirtualDesktop* pDesktopFallback) override
    {
    }

    virtual void CurrentVirtualDesktopChanged(Win10::IVirtualDesktop* pDesktopOld, Win10::IVirtualDesktop* pDesktopNew) override
    {
        SendMessage(*this, WM_UPDATE, 0, reinterpret_cast<LPARAM>(pDesktopNew));
    }

    virtual void VirtualDesktopNameChanged(Win10::IVirtualDesktop* pDesktop, HSTRING name) override
    {
        SendMessage(*this, WM_UPDATE, 0, reinterpret_cast<LPARAM>(pDesktop));
    }

private: // IVDNotification Win11::IVirtualDesktopNotification
    virtual void VirtualDesktopCreated(Win11::IVirtualDesktop* pDesktop) override
    {
    }

    virtual void VirtualDesktopDestroyed(Win11::IVirtualDesktop* pDesktopDestroyed, Win11::IVirtualDesktop* pDesktopFallback) override
    {
    }

    virtual void VirtualDesktopMoved(Win11::IVirtualDesktop* pDesktop, int64_t oldIndex, int64_t newIndex) override
    {
    }

    virtual void VirtualDesktopNameChanged(Win11::IVirtualDesktop* pDesktop, HSTRING name) override
    {
        // TODO Support Win11
        //SendMessage(*this, WM_UPDATE, 0, reinterpret_cast<LPARAM>(pDesktop));
    }

    virtual void CurrentVirtualDesktopChanged(Win11::IVirtualDesktop* pDesktopOld, Win11::IVirtualDesktop* pDesktopNew) override
    {
        // TODO Support Win11
        //SendMessage(*this, WM_UPDATE, 0, reinterpret_cast<LPARAM>(pDesktopNew));
    }

private:
    CComPtr<IServiceProvider> m_pServiceProvider;
    CComPtr<Win10::IVirtualDesktopManagerInternal> m_pDesktopManagerInternal;

    VDNotification m_notifications;
};

extern "C"
{
    __declspec(dllexport) HWND CreateWidget(HWND hWndParent, const WidgetParams* params) { return *DesktopsWidgetWindow::Create(hWndParent, params); }
    __declspec(dllexport) HWND CreateDesktopNameWidget(HWND hWndParent, const WidgetParams* params) { return *DesktopNameWidgetWindow::Create(hWndParent, params); }
}
