#pragma once

#include <Windows.h>
#include <memory>

struct HKEYDeleter
{
    typedef HKEY pointer;
    void operator()(HKEY hKey) const noexcept { RegCloseKey(hKey); }
};

typedef std::unique_ptr<HKEY, HKEYDeleter> HKEYPtr;
//typedef std::unique_ptr<HKEY, HandleDeleter<HKEY, RegCloseKey>> HKEYPtr; TODO Return type is LSTATUS not BOOL

inline LRESULT RegGetString(_In_ HKEY hKey, _In_opt_ LPCTSTR lpValueName, LPTSTR pStr, DWORD size)
{
    size *= sizeof(TCHAR);
    LRESULT ret = RegQueryValueEx(hKey, lpValueName, nullptr, nullptr, (LPBYTE) pStr, &size);
    return ret;
}

inline LRESULT RegGetStringA(_In_ HKEY hKey, _In_opt_ LPCSTR lpValueName, LPSTR pStr, DWORD size)
{
    size *= sizeof(char);
    LRESULT ret = RegQueryValueExA(hKey, lpValueName, nullptr, nullptr, (LPBYTE) pStr, &size);
    return ret;
}

inline DWORD RegGetDWORD(_In_ HKEY hKey, _In_opt_ LPCTSTR lpValueName, DWORD defvalue)
{
    DWORD value = defvalue;
    DWORD valuesize = sizeof(value);
    LRESULT ret = RegQueryValueEx(hKey, lpValueName, nullptr, nullptr, (LPBYTE) &value, &valuesize);
    return (ret == ERROR_SUCCESS) ? value : defvalue;
}
