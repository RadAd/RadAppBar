#pragma once

#include <Windows.h>
#include <string>

// Recommended to add this to compiler options:
// /d1trimfile:"$(SolutionDir)\"

struct SourceLocation
{
    SourceLocation(int line, const TCHAR* file, const TCHAR* function, const TCHAR* funcsig)
        : line(line)
        , file(file)
        , function(function)
        , funcsig(funcsig)
    {
    }

    int line;
    const TCHAR* file;
    const TCHAR* function;
    const TCHAR* funcsig;
};

#define SRC_LOC SourceLocation(__LINE__, TEXT(__FILE__), TEXT(__FUNCTION__), TEXT(__FUNCSIG__))

#undef ERROR
enum class LogLevel
{
    DEBUG,
    INFO,
    WARN,
    ERROR,
    NONE,
};

extern LogLevel LogDebug;
extern LogLevel LogStdout;
extern LogLevel LogMsgBox;

void InitLog(HWND hWnd, LPCTSTR app_name);

void Log(LogLevel level, LPCTSTR expr, const SourceLocation& src_loc);
void Log(LogLevel level, LPCTSTR expr, const SourceLocation& src_loc, LPCTSTR format, ...);

#define LOG(l, x, ...) Log(l, x, SRC_LOC, __VA_ARGS__)
#define CHECK(l, b, ...) if (!(b)) { LOG(l, TEXT(#b), __VA_ARGS__); }
#define CHECK_RET(l, b, r, ...) if (!(b)) { LOG(l, TEXT(#b), __VA_ARGS__); return r; }
#define CHECK_THROW(l, b, t, ...) if (!(b)) { LOG(l, TEXT(#b), __VA_ARGS__); throw t; }

std::wstring FormatErrorMessage(HRESULT hr);
std::wstring FormatErrorMessage(HRESULT hr, LPCTSTR msg);    // TODO suport varargs

#define CHECKLE(l, b, ...) if (!(b)) { LOG(l, TEXT(#b), FormatErrorMessage(GetLastError(), __VA_ARGS__).c_str()); }
#define CHECKLE_RET(l, b, r, ...) if (!(b)) { LOG(l, TEXT(#b), FormatErrorMessage(GetLastError(), __VA_ARGS__).c_str()); return r; }
#define CHECKHR(l, b, ...) { HRESULT hr = b; if (FAILED(hr)) { LOG(l, TEXT(#b), FormatErrorMessage(hr, __VA_ARGS__).c_str()); } }
#define CHECKHR_RET(l, b, r, ...) { HRESULT hr = b; if (FAILED(hr)) { LOG(l, TEXT(#b), FormatErrorMessage(GetLastError(), __VA_ARGS__).c_str()); return r; } }
