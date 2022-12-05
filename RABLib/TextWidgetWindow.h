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

    void HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

    void OnCreate(const LPCREATESTRUCT lpCreateStruct, LRESULT* pResult);

    virtual void OnDraw(const PAINTSTRUCT* pps) const override;

    bool SetText(LPCTSTR strText);
    bool SetIcon(HICON hIcon);
    HICON GetIcon() const { return m_hIcon; }

private:
    HICON m_hIcon = NULL;
    TCHAR m_strText[1024] = TEXT("");
};
