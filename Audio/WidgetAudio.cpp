#define STRICT
#define WIN32_LEAN_AND_MEAN
#include "..\RABLib\TextWidgetWindow.h"
#include "..\RABLib\Windowxx.h"
#include "..\RABLib\reg_utils.h"

#include "..\API\RadAppBar.h"

#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>

//#include <tchar.h>

#include <AtlBase.h>
//#include <AtlWin.h>
#include <AtlCom.h>
//#include <AtlCtl.h>
//#include <AtlStr.h>
//#include <AtlColl.h>
//#include <AtlPath.h>

#define WM_DEFAULTDEVICECHANGED (WM_USER + 100)
#define WM_AUDIOENDPOINTVOLUMECHANGED (WM_USER + 101)

class MMNotificationClient :
    public CComObjectRootEx<ATL::CComSingleThreadModel>,
    public IMMNotificationClient
{
    BEGIN_COM_MAP(MMNotificationClient)
        COM_INTERFACE_ENTRY_IID(IID_IUnknown, IMMNotificationClient)
        COM_INTERFACE_ENTRY(IMMNotificationClient)
    END_COM_MAP()

public:
    void Init(HWND hWnd)
    {
        m_hWnd = hWnd;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(
        _In_  LPCWSTR pwstrDeviceId,
        _In_  DWORD dwNewState) override
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(
        _In_  LPCWSTR pwstrDeviceId) override
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(
        _In_  LPCWSTR pwstrDeviceId) override
    {
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
        _In_  EDataFlow flow,
        _In_  ERole role,
        _In_  LPCWSTR pwstrDefaultDeviceId) override
    {
        PostMessage(m_hWnd, WM_DEFAULTDEVICECHANGED, 0, 0);
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(
        _In_  LPCWSTR pwstrDeviceId,
        _In_  const PROPERTYKEY key) override
    {
        return S_OK;
    }

private:
    HWND m_hWnd = NULL;
};

class AudioEndpointVolumeCallback :
    public CComObjectRootEx<ATL::CComSingleThreadModel>,
    public IAudioEndpointVolumeCallback
{
    BEGIN_COM_MAP(AudioEndpointVolumeCallback)
        COM_INTERFACE_ENTRY_IID(IID_IUnknown, IAudioEndpointVolumeCallback)
        COM_INTERFACE_ENTRY(IAudioEndpointVolumeCallback)
    END_COM_MAP()

public:
    void Init(HWND hWnd)
    {
        m_hWnd = hWnd;
    }

    virtual HRESULT STDMETHODCALLTYPE OnNotify(
        PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) override
    {
        PostMessage(m_hWnd, WM_AUDIOENDPOINTVOLUMECHANGED, 0, 0);
        return S_OK;
    }

private:
    HWND m_hWnd = NULL;
};

class VolumeWidgetWindow : public TextWidgetWindow
{
    typedef TextWidgetWindow Base;
    friend WindowManager<VolumeWidgetWindow>;
public:
    static WidgetWindow* Create(HWND hWndParent, const WidgetParams* params)
    {
        return WindowManager<VolumeWidgetWindow>::Create(hWndParent, params->Name, const_cast<WidgetParams*>(params));
    }

protected:
    VolumeWidgetWindow()
    {
        ATL::CComObject<MMNotificationClient>* pClient;
        ATLVERIFY(SUCCEEDED(CComObject<MMNotificationClient>::CreateInstance(&pClient)));
        m_pClient = pClient;

        ATL::CComObject<AudioEndpointVolumeCallback>* pNotify;
        ATLVERIFY(SUCCEEDED(CComObject<AudioEndpointVolumeCallback>::CreateInstance(&pNotify)));
        m_pNotify = pNotify;

        ATL::CComPtr<IMMDeviceEnumerator> pEnumerator;
        ATLENSURE(SUCCEEDED(pEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER)));

        ATLENSURE(SUCCEEDED(pEnumerator->RegisterEndpointNotificationCallback(pClient)));

        Register();
        Update();
    }

    void OnCreate(const LPCREATESTRUCT lpCreateStruct, LRESULT* pResult)
    {
        const WidgetParams* params = reinterpret_cast<WidgetParams*>(lpCreateStruct->lpCreateParams);

        TCHAR szMute[MAX_PATH] = TEXT("");
        RegGetString(params->hWidget, TEXT("Mute"), szMute, ARRAYSIZE(szMute));

        m_hIcon = GetIcon();
        m_hMute = (HICON) LoadImage(NULL, szMute, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT);

        m_pClient->Init(*this);
        m_pNotify->Init(*this);
    }

    void HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam, LRESULT* pResult)
    {
        switch (uMsg)
        {
        case WM_DEFAULTDEVICECHANGED: Register(); if (Update()) FixSize(); break;
        case WM_AUDIOENDPOINTVOLUMECHANGED: if (Update()) FixSize(); break;
        }

        switch (uMsg)
        {
            HANDLE_NOT(WM_CREATE, Base::HandleMessage, OnCreate);
            HANDLE_DEF(Base::HandleMessage);
        }
    }

    void Register()
    {
        if (m_pAudioEndpointVolume)
        {
            ATLENSURE(SUCCEEDED(m_pAudioEndpointVolume->UnregisterControlChangeNotify(m_pNotify)));
            m_pAudioEndpointVolume.Release();
        }

        ATL::CComPtr<IMMDeviceEnumerator> pEnumerator;
        ATLENSURE(SUCCEEDED(pEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER)));

        // Get default audio-rendering device.
        ATL::CComPtr<IMMDevice> pDevice;
        ATLENSURE(SUCCEEDED(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice)));

        ATL::CComPtr<IAudioEndpointVolume> pAudioEndpointVolume;
        ATLENSURE(SUCCEEDED(pDevice->Activate(
            __uuidof(IAudioEndpointVolume),
            CLSCTX_ALL,
            NULL,
            (void**) &pAudioEndpointVolume)));

        m_pAudioEndpointVolume = pAudioEndpointVolume;
        ATLENSURE(SUCCEEDED(pAudioEndpointVolume->RegisterControlChangeNotify(m_pNotify)));
    }

    bool Update()
    {
        if (m_hMute != NULL)
        {
            BOOL Mute;
            ATLENSURE_RETURN_VAL(SUCCEEDED(m_pAudioEndpointVolume->GetMute(&Mute)), false);

            SetIcon(Mute ? m_hMute : m_hIcon);
        }

        float MasterVolume;
        ATLENSURE_RETURN_VAL(SUCCEEDED(m_pAudioEndpointVolume->GetMasterVolumeLevelScalar(&MasterVolume)), false);

        TCHAR strText[1024];
        _stprintf_s(strText, TEXT("%.0f%%"), MasterVolume * 100);
        return SetText(strText);
    }

private:
    ATL::CComPtr<MMNotificationClient> m_pClient;
    ATL::CComPtr<IAudioEndpointVolume> m_pAudioEndpointVolume;
    ATL::CComPtr<AudioEndpointVolumeCallback> m_pNotify;
    HICON m_hIcon = NULL;
    HICON m_hMute = NULL;
};

class DeviceWidgetWindow : public TextWidgetWindow
{
    typedef TextWidgetWindow Base;
    friend WindowManager<DeviceWidgetWindow>;
public:
    static WidgetWindow* Create(HWND hWndParent, const WidgetParams* params)
    {
        return WindowManager<DeviceWidgetWindow>::Create(hWndParent, params->Name, const_cast<WidgetParams*>(params));
    }

protected:
    DeviceWidgetWindow()
    {
        ATL::CComObject<MMNotificationClient>* pClient;
        ATLVERIFY(SUCCEEDED(CComObject<MMNotificationClient>::CreateInstance(&pClient)));
        m_pClient = pClient;

        ATL::CComPtr<IMMDeviceEnumerator> pEnumerator;
        ATLENSURE(SUCCEEDED(pEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER)));

        ATLENSURE(SUCCEEDED(pEnumerator->RegisterEndpointNotificationCallback(pClient)));

        Update();
    }

    BOOL OnCreate(const LPCREATESTRUCT lpCreateStruct, LRESULT* pResult)
    {
        m_pClient->Init(*this);
        return TRUE;
    }

    void HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam, LRESULT* pResult)
    {
        switch (uMsg)
        {
        case WM_DEFAULTDEVICECHANGED: if (Update()) FixSize(); break;
        }

        switch (uMsg)
        {
            HANDLE_NOT(WM_CREATE, Base::HandleMessage, OnCreate);
            HANDLE_DEF(Base::HandleMessage);
        }
    }

    bool Update()
    {
        ATL::CComPtr<IMMDeviceEnumerator> pEnumerator;
        ATLENSURE_RETURN_VAL(SUCCEEDED(pEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER)), false);

        // Get default audio-rendering device.
        ATL::CComPtr<IMMDevice> pDevice;
        ATLENSURE_RETURN_VAL(SUCCEEDED(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice)), false);

        // Open device properties
        ATL::CComPtr<IPropertyStore> pProps;
        ATLENSURE_RETURN_VAL(SUCCEEDED(pDevice->OpenPropertyStore(STGM_READ, &pProps)), false);

        // Get the device's friendly name
        //PROPERTYKEY key = PKEY_Device_FriendlyName;
        PROPERTYKEY key = PKEY_Device_DeviceDesc;

        PROPVARIANT varName;
        PropVariantInit(&varName);
        ATLENSURE_RETURN_VAL(SUCCEEDED(pProps->GetValue(key, &varName)), false);

        ATLENSURE_RETURN_VAL(varName.vt == VT_LPWSTR, false);
        bool ret = SetText(varName.pwszVal);
        ATLENSURE_RETURN_VAL(SUCCEEDED(PropVariantClear(&varName)), false);

        return ret;
    }

private:
    ATL::CComPtr<MMNotificationClient> m_pClient;
};

extern "C"
{
    __declspec(dllexport) HWND CreateWidgetVolume(HWND hWndParent, const WidgetParams* params) { return *VolumeWidgetWindow::Create(hWndParent, params); }
    __declspec(dllexport) HWND CreateWidgetDevice(HWND hWndParent, const WidgetParams* params) { return *DeviceWidgetWindow::Create(hWndParent, params); }
}
