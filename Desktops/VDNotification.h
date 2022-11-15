#pragma once

#include "Win10Desktops.h"

class IVDNotification
{
public:
    virtual void VirtualDesktopCreated(Win10::IVirtualDesktop* pDesktop) = 0;
    virtual void VirtualDesktopDestroyed(Win10::IVirtualDesktop* pDesktopDestroyed, Win10::IVirtualDesktop* pDesktopFallback) = 0;
    virtual void CurrentVirtualDesktopChanged(Win10::IVirtualDesktop* pDesktopOld, Win10::IVirtualDesktop* pDesktopNew) = 0;
};

class VirtualDesktopNotification : public Win10::IVirtualDesktopNotification
{
private:
    IVDNotification* _notify;
    ULONG _referenceCount;
public:
    VirtualDesktopNotification(IVDNotification* notify);

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;
    STDMETHODIMP_(DWORD) AddRef() override;
    STDMETHODIMP_(DWORD) STDMETHODCALLTYPE Release() override;

    STDMETHODIMP VirtualDesktopCreated(Win10::IVirtualDesktop* pDesktop) override;
    STDMETHODIMP VirtualDesktopDestroyBegin(Win10::IVirtualDesktop* pDesktopDestroyed, Win10::IVirtualDesktop* pDesktopFallback) override;
    STDMETHODIMP VirtualDesktopDestroyFailed(Win10::IVirtualDesktop* pDesktopDestroyed, Win10::IVirtualDesktop* pDesktopFallback) override;
    STDMETHODIMP VirtualDesktopDestroyed(Win10::IVirtualDesktop* pDesktopDestroyed, Win10::IVirtualDesktop* pDesktopFallback) override;
    STDMETHODIMP ViewVirtualDesktopChanged(IApplicationView* pView) override;
    STDMETHODIMP CurrentVirtualDesktopChanged(Win10::IVirtualDesktop* pDesktopOld, Win10::IVirtualDesktop* pDesktopNew) override;
};
