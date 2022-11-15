#define STRICT
#define WIN32_LEAN_AND_MEAN
#include "..\RABLib\TextWidgetWindow.h"
#include "..\RABLib\Windowxx.h"

#include "..\API\RadAppBar.h"

//#include <tchar.h>

#define WM_UPDATE (WM_USER + 151)
HWND g_hwndMonitor = NULL;

void CALLBACK WinEventProc(
    HWINEVENTHOOK /*hook*/,
    DWORD event,
    HWND hWnd,
    LONG idObject,
    LONG idChild,
    DWORD /*idEventThread*/,
    DWORD /*time*/)
{
    if (idObject == OBJID_WINDOW &&
        idChild == CHILDID_SELF)
    {
        switch (event)
        {
        case EVENT_SYSTEM_FOREGROUND:
            SendMessage(g_hwndMonitor, WM_UPDATE, reinterpret_cast<UINT_PTR>(hWnd), 0);
            break;

        case EVENT_OBJECT_NAMECHANGE:
            //if (GetAncestor(hWnd, GA_ROOTOWNER) == hWnd)
            if (hWnd == GetForegroundWindow())
                SendMessage(g_hwndMonitor, WM_UPDATE, reinterpret_cast<UINT_PTR>(hWnd), 0);
            break;
        }
    }
}

class FgWindowWidgetWindow : public TextWidgetWindow
{
    typedef TextWidgetWindow Base;
    friend WindowManager<FgWindowWidgetWindow>;
public:
    static WidgetWindow* Create(HWND hWndParent, const WidgetParams* params)
    {
        return WindowManager<FgWindowWidgetWindow>::Create(hWndParent, params->Name, const_cast<WidgetParams*>(params));
    }

protected:
    FgWindowWidgetWindow()
    {
        Update(GetForegroundWindow());

        m_hook = SetWinEventHook(
            EVENT_SYSTEM_FOREGROUND, //EVENT_MIN,
            EVENT_OBJECT_NAMECHANGE, //EVENT_MAX,
            nullptr,
            WinEventProc,
            0,
            0,
            WINEVENT_OUTOFCONTEXT);
    }

    ~FgWindowWidgetWindow()
    {
        UnhookWinEvent(m_hook); // TODO Use typedef std::unique_ptr<HWINEVENTHOOK, HandleDeleter<HWINEVENTHOOK, HWINEVENTHOOK, UnhookWinEvent>> WinEventHookPtr;
    }

    bool Update(HWND hWnd)
    {
        TCHAR strText[1024];
        GetWindowText(hWnd, strText, ARRAYSIZE(strText));
        const bool ret1 = SetText(strText);

        HICON hIcon = NULL;
        if (GetWindowStyle(hWnd) & WS_CAPTION)
        {
            // TODO Use Timeout
            if (hIcon == NULL)
                hIcon = (HICON) SendMessage(hWnd, WM_GETICON, ICON_SMALL2, 0);
            if (hIcon == NULL)
                hIcon = (HICON) SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
            if (hIcon == NULL)
                hIcon = (HICON) SendMessage(hWnd, WM_GETICON, ICON_BIG, 0);
            if (hIcon == NULL)
                hIcon = (HICON) GetClassLongPtr(hWnd, GCLP_HICONSM);
            if (hIcon == NULL)
                hIcon = (HICON) GetClassLongPtr(hWnd, GCLP_HICON);
        }
        const bool ret2 = SetIcon(hIcon);

        return ret1 || ret2;
    }

protected:
    void HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override
    {
        switch (uMsg)
        {
        case WM_UPDATE:
            if (Update(reinterpret_cast<HWND>(wParam)))
                FixSize();
            *pResult = 0;
            break;
        default:
            Base::HandleMessage(uMsg, wParam, lParam, pResult);
            break;
        }
    }

private:
    HWINEVENTHOOK m_hook;
};

extern "C"
{
    __declspec(dllexport) HWND CreateWidget(HWND hWndParent, const WidgetParams* params) { return g_hwndMonitor = *FgWindowWidgetWindow::Create(hWndParent, params); }
}
