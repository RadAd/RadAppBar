#include "VDNotification.h"

VirtualDesktopNotification::VirtualDesktopNotification(IVDNotification* notify)
    : _notify(notify), _referenceCount(0)
{
}

STDMETHODIMP VirtualDesktopNotification::QueryInterface(REFIID riid, void** ppvObject)
{
    // Always set out parameter to NULL, validating it first.
    if (!ppvObject)
        return E_INVALIDARG;
    *ppvObject = NULL;

    if (riid == IID_IUnknown || riid == Win10::IID_IVirtualDesktopNotification)
    {
        // Increment the reference count and return the pointer.
        *ppvObject = (LPVOID)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(DWORD) VirtualDesktopNotification::AddRef()
{
    return InterlockedIncrement(&_referenceCount);
}

STDMETHODIMP_(DWORD) STDMETHODCALLTYPE VirtualDesktopNotification::Release()
{
    ULONG result = InterlockedDecrement(&_referenceCount);
    if (result == 0)
    {
        delete this;
    }
    return 0;
}

STDMETHODIMP VirtualDesktopNotification::VirtualDesktopCreated(Win10::IVirtualDesktop* pDesktop)
{
    _notify->VirtualDesktopCreated(pDesktop);
    return S_OK;
}

STDMETHODIMP VirtualDesktopNotification::VirtualDesktopDestroyBegin(Win10::IVirtualDesktop* pDesktopDestroyed, Win10::IVirtualDesktop* pDesktopFallback)
{
    return S_OK;
}

STDMETHODIMP VirtualDesktopNotification::VirtualDesktopDestroyFailed(Win10::IVirtualDesktop* pDesktopDestroyed, Win10::IVirtualDesktop* pDesktopFallback)
{
    return S_OK;
}

STDMETHODIMP VirtualDesktopNotification::VirtualDesktopDestroyed(Win10::IVirtualDesktop* pDesktopDestroyed, Win10::IVirtualDesktop* pDesktopFallback)
{
    _notify->VirtualDesktopDestroyed(pDesktopDestroyed, pDesktopFallback);
    return S_OK;
}

STDMETHODIMP VirtualDesktopNotification::ViewVirtualDesktopChanged(IApplicationView* pView)
{
    return S_OK;
}

STDMETHODIMP VirtualDesktopNotification::CurrentVirtualDesktopChanged(Win10::IVirtualDesktop* pDesktopOld, Win10::IVirtualDesktop* pDesktopNew)
{
    _notify->CurrentVirtualDesktopChanged(pDesktopOld, pDesktopNew);
    return S_OK;
}
