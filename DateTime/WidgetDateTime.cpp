#define STRICT
#define WIN32_LEAN_AND_MEAN
#include "..\RABLib\TextWidgetWindow.h"
#include "..\RABLib\Windowxx.h"

#include "..\API\RadAppBar.h"

//#include <tchar.h>

class DateWidgetWindow : public TextWidgetWindow
{
    typedef TextWidgetWindow Base;
    friend WindowManager<DateWidgetWindow>;
public:
    static WidgetWindow* Create(HWND hWndParent, const WidgetParams* params)
    {
        return WindowManager<DateWidgetWindow>::Create(hWndParent, params->Name, const_cast<WidgetParams*>(params));
    }

protected:
    DateWidgetWindow()
    {
        Update();
    }

    bool Update()
    {
        TCHAR strText[1024];
        // Format: https://learn.microsoft.com/en-us/windows/win32/intl/day--month--year--and-era-format-pictures
        //GetDateFormat(LOCALE_USER_DEFAULT, 0, nullptr, _T("dddd"), m_strText, ARRAYSIZE(m_strText));
        GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, nullptr, nullptr, strText, ARRAYSIZE(strText));
        return SetText(strText);
    }
};

class TimeWidgetWindow : public TextWidgetWindow
{
    typedef TextWidgetWindow Base;
    friend WindowManager<TimeWidgetWindow>;
public:
    static WidgetWindow* Create(HWND hWndParent, const WidgetParams* params)
    {
        return WindowManager<TimeWidgetWindow>::Create(hWndParent, params->Name, const_cast<WidgetParams*>(params));
    }

protected:
    TimeWidgetWindow()
    {
        Update();
    }

    BOOL OnCreate(const LPCREATESTRUCT lpCreateStruct, LRESULT* pResult)
    {
        SetTimer(*this, 1, 1000, nullptr);
        return TRUE;
    }

    void OnTimer(UINT id)
    {
        if (Update())
            FixSize();
    }

    void HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam, LRESULT* pResult)
    {
        switch (uMsg)
        {
            HANDLE_NOT(WM_CREATE, Base::HandleMessage, OnCreate);
            HANDLE_NOT(WM_TIMER, Base::HandleMessage, OnTimer);
            HANDLE_DEF(Base::HandleMessage);
        }
    }

    bool Update()
    {
        TCHAR strText[1024];
        GetTimeFormat(LOCALE_USER_DEFAULT, 0 /*TIME_NOSECONDS*/, nullptr, nullptr, strText, ARRAYSIZE(strText));
        return SetText(strText);
    }
};

extern "C"
{
    __declspec(dllexport) HWND CreateWidgetDate(HWND hWndParent, const WidgetParams* params) { return *DateWidgetWindow::Create(hWndParent, params); }
    __declspec(dllexport) HWND CreateWidgetTime(HWND hWndParent, const WidgetParams* params) { return *TimeWidgetWindow::Create(hWndParent, params); }
}
