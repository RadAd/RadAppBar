#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <AtlBase.h>
//#include <AtlWin.h>
#include <AtlCom.h>
//#include <AtlCtl.h>
//#include <AtlStr.h>
//#include <AtlColl.h>
//#include <AtlPath.h>

HINSTANCE g_hInstance = NULL;

class CModule : public ATL::CAtlDllModuleT<CModule>
{
public:
};

CModule _AtlModule;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    _AtlModule.DllMain(ul_reason_for_call, lpReserved);

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hModule;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
