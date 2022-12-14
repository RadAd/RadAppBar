#include "Logging.h"

#include <tchar.h>

namespace
{
    HWND g_hLogWnd = NULL;
    LPCTSTR g_LogAppName = nullptr;

    LPCTSTR LogLevelStr(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG: return TEXT("DEBUG");
        case LogLevel::INFO: return TEXT("INFO");
        case LogLevel::WARN: return TEXT("WARN");
        case LogLevel::ERROR: return TEXT("ERROR");
        default: return TEXT("UNKOWN");
        }
    }

    UINT LogLevelIcon(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::DEBUG: return MB_ICONQUESTION;
        case LogLevel::INFO: return MB_ICONINFORMATION;
        case LogLevel::WARN: return MB_ICONWARNING;
        case LogLevel::ERROR: return MB_ICONERROR;
        default: return MB_ICONERROR;
        }
    }
}

LogLevel LogDebug = LogLevel::DEBUG;
LogLevel LogStdout = LogLevel::NONE;
LogLevel LogMsgBox = LogLevel::ERROR;

void InitLog(HWND hWnd, LPCTSTR app_name)
{
    g_hLogWnd = hWnd;
    g_LogAppName = app_name;
}

void Log(LogLevel level, LPCTSTR expr, const SourceLocation& src_loc)
{
    if (level >= LogDebug)
    {
        TCHAR msg[1024];
        _stprintf_s(msg, TEXT("%s: %s {%s %s(%d)}\n"), LogLevelStr(level), expr, src_loc.funcsig, src_loc.file, src_loc.line);
        OutputDebugString(msg);
    }
    if (level >= LogStdout) _ftprintf(stdout, TEXT("%s: %s {%s %s(%d)}\n"), LogLevelStr(level), expr, src_loc.funcsig, src_loc.file, src_loc.line);
    if (level >= LogMsgBox)
    {
        TCHAR msg[1024];
        _stprintf_s(msg, TEXT("%s\n%s\n%s(%d)"), expr, src_loc.funcsig, src_loc.file, src_loc.line);
        MessageBox(g_hLogWnd, msg, g_LogAppName, MB_OK | LogLevelIcon(level));
    }
}

void Log(LogLevel level, LPCTSTR expr, const SourceLocation& src_loc, LPCTSTR format, ...)
{
    va_list args;
    va_start(args, format);
    if (level >= LogDebug || level >= LogStdout)
    {
        TCHAR fullformat[1024];
        _stprintf_s(fullformat, TEXT("%s: %s {%s %s(%d)} - %s\n"), LogLevelStr(level), expr, src_loc.funcsig, src_loc.file, src_loc.line, format);
        if (level >= LogDebug)
        {
            TCHAR msg[1024];
            _vstprintf_s(msg, fullformat, args);
            OutputDebugString(msg);
        }
        if (level >= LogStdout) _vftprintf(stdout, fullformat, args);
    }
    if (level >= LogMsgBox)
    {
        TCHAR fullformat[1024];
        _stprintf_s(fullformat, TEXT("%s\n%s\n%s\n%s(%d)"), format, expr, src_loc.funcsig, src_loc.file, src_loc.line);
        TCHAR msg[1024];
        _vstprintf_s(msg, fullformat, args);
        MessageBox(g_hLogWnd, msg, g_LogAppName, MB_OK | LogLevelIcon(level));
    }
    va_end(args);
}

std::wstring FormatErrorMessage(HRESULT hr)
{
    TCHAR   wszMsgBuff[512];
    DWORD   dwChars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        hr,
        0,
        wszMsgBuff,
        ARRAYSIZE(wszMsgBuff),
        NULL);
    while (dwChars > 0 && (wszMsgBuff[dwChars - 1] == TEXT('\n') || wszMsgBuff[dwChars - 1] == TEXT('\r')))
        --dwChars;
    wszMsgBuff[dwChars] = TEXT('\0');

    TCHAR   wszFullMsgBuff[1024];
    _stprintf_s(wszFullMsgBuff, TEXT("Error (0x%08X): %s"), hr, wszMsgBuff);
    return wszFullMsgBuff;
}

std::wstring FormatErrorMessage(HRESULT hr, LPCTSTR msg)    // TODO suport varargs
{
    TCHAR   wszMsgBuff[512];
    DWORD   dwChars = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        hr,
        0,
        wszMsgBuff,
        ARRAYSIZE(wszMsgBuff),
        NULL);
    while (dwChars > 0 && (wszMsgBuff[dwChars - 1] == TEXT('\n') || wszMsgBuff[dwChars - 1] == TEXT('\r')))
        --dwChars;
    wszMsgBuff[dwChars] = TEXT('\0');

    TCHAR   wszFullMsgBuff[1024];
    _stprintf_s(wszFullMsgBuff, TEXT("Error (0x%08X) - %s: %s"), hr, msg, wszMsgBuff);
    return wszFullMsgBuff;
}
