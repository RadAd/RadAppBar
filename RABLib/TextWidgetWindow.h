#pragma once

#include "WidgetWindow.h"

struct WidgetParams;

class TextWidgetWindow : public WidgetWindow
{
    friend WindowManager<TextWidgetWindow>;
public:
    static WidgetWindow* Create(HWND hWndParent, const WidgetParams* params);

protected:
    virtual SIZE CalcSize() override;

    virtual void OnDraw(const PAINTSTRUCT* pps) const override;

    bool SetText(LPCTSTR strText);
    bool SetIcon(HICON hIcon);

private:
    HICON m_hIcon = NULL;
    TCHAR m_strText[1024] = TEXT("");
};
