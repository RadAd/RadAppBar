#define STRICT
#define WIN32_LEAN_AND_MEAN
#include "..\RABLib\TextWidgetWindow.h"
#include "..\RABLib\Windowxx.h"
#include "..\RABLib\memory_utils.h"
#include "..\RABLib\Logging.h"
#include "..\RABLib\Format.h"
#include "..\RABLib\reg_utils.h"

#include "..\API\RadAppBar.h"

//#include <tchar.h>
#include <Pdh.h>
#include <PdhMsg.h>
#include <vector>
#include <numeric>
#include <string>

struct QueryDeleter
{
    typedef PDH_HQUERY pointer;
    void operator()(PDH_HQUERY hQuery) const noexcept { PdhCloseQuery(hQuery); }
};

inline LONG Ignore(LONG e, std::initializer_list<LONG> ignored)
{
    for (LONG candidate : ignored)
    {
        if (e == candidate)
            return S_OK;
    }
    return e;
}

inline TCHAR GetPrefix(double& value)
{
    if (value > (1024 * 90 / 100))
    {
        const TCHAR PrefixSet[] = TEXT(" KMGTPE");
        const TCHAR* CurrentPrefix = PrefixSet;
        while (value > (1024 * 90 / 100))
        {
            value /= 1024;
            ++CurrentPrefix;
        }
        return *CurrentPrefix;
    }
    else
        return TEXT(' ');
}

std::wstring FormatPdhValue(DWORD dwFormat, DWORD dwType, PDH_FMT_COUNTERVALUE Value, LPCTSTR szName, LPCTSTR szUnit)
{
    switch (dwFormat)
    {
    case PDH_FMT_DOUBLE:
    {
        double value = Value.doubleValue;
        TCHAR prefix = GetPrefix(value);
        switch (dwType & 0xF0000000)
        {
        case PERF_DISPLAY_PER_SEC:  return Format(TEXT("%s%.1f %c%s/s"), szName, value, prefix, szUnit);
        case PERF_DISPLAY_PERCENT:  return Format(TEXT("%s%.0f%%"), szName, Value.doubleValue);
        case PERF_DISPLAY_SECONDS:  return Format(TEXT("%s%.1f %c%s s"), szName, value, prefix, szUnit);
        default:                    return Format(TEXT("%s%.1f %c%s"), szName, value, prefix, szUnit);
        }
    }
    break;
    case PDH_FMT_LONG:
    {
        switch (dwType & 0xF0000000)
        {
        case PERF_DISPLAY_PER_SEC:  return Format(TEXT("%s%d %s/s"), szName, Value.longValue, szUnit);
        case PERF_DISPLAY_PERCENT:  return Format(TEXT("%s%d%%"), szName, Value.longValue);
        case PERF_DISPLAY_SECONDS:  return Format(TEXT("%s%d %s s"), szName, Value.longValue, szUnit);
        default:                    return Format(TEXT("%s%d %s"), szName, Value.longValue, szUnit);
        }
    }
    break;
    default:
        return Format(TEXT("Unknown format: %d"), dwFormat);
    }
}

enum TIMER
{
    TIMER_UPDATE,
};

HWND g_hwndMonitor = NULL;

class PerfWidgetWindow : public TextWidgetWindow
{
    typedef TextWidgetWindow Base;
    friend WindowManager<PerfWidgetWindow>;
public:
    static WidgetWindow* Create(HWND hWndParent, const WidgetParams* params)
    {
        return WindowManager<PerfWidgetWindow>::Create(hWndParent, params->Name, const_cast<WidgetParams*>(params));
    }

protected:
    void HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override
    {
        InitLog(*this, TEXT("WidgetPerf"));
        switch (uMsg)
        {
            HANDLE_NOT(WM_CREATE, Base::HandleMessage, OnCreate);
            HANDLE_NOT(WM_TIMER, Base::HandleMessage, OnTimer);
            HANDLE_DEF(Base::HandleMessage);
        }
    }

    void OnCreate(const LPCREATESTRUCT lpCreateStruct, LRESULT* pResult)
    {
        const WidgetParams* params = reinterpret_cast<WidgetParams*>(lpCreateStruct->lpCreateParams);

        TCHAR szCounterPath[1024] = TEXT("\\Processor(_Total)\\% Processor Time");
        RegGetString(params->hWidget, TEXT("Counter"), szCounterPath, ARRAYSIZE(szCounterPath));
        RegGetString(params->hWidget, TEXT("Name"), m_szName, ARRAYSIZE(m_szName));
        RegGetString(params->hWidget, TEXT("Unit"), m_szUnit, ARRAYSIZE(m_szUnit));

        CHECKHR(LogLevel::ERROR, PdhOpenQuery(nullptr, 0, &m_hQuery));
        CHECKHR(LogLevel::ERROR, PdhAddEnglishCounter(-m_hQuery, szCounterPath, 0, &m_hCounter));
        CHECKHR(LogLevel::ERROR, PdhCollectQueryData(-m_hQuery));

        CHECKLE(LogLevel::ERROR, SetTimer(*this, TIMER_UPDATE, 1000, nullptr));
    }

    void OnTimer(UINT id)
    {
        switch (id)
        {
        case TIMER_UPDATE:
        {
            CHECKHR(LogLevel::ERROR, PdhCollectQueryData(m_hQuery.get()));

            DWORD dwType;
            PDH_FMT_COUNTERVALUE Value;
            CHECKHR(LogLevel::ERROR, Ignore(PdhGetFormattedCounterValue(m_hCounter, m_dwFormat, &dwType, &Value), { PDH_STATUS(PDH_CALC_NEGATIVE_DENOMINATOR) }));

            DWORD dwBufferSize = 0;
            DWORD dwItemCount = 0;
            PDH_INVALID_ARGUMENT;
            CHECKHR(LogLevel::ERROR, Ignore(PdhGetFormattedCounterArray(m_hCounter, m_dwFormat, &dwBufferSize, &dwItemCount, nullptr), { PDH_STATUS(PDH_MORE_DATA) }));
            //assert(dwItemCount * sizeof(PDH_FMT_COUNTERVALUE) == dwBufferSize);

            if (dwItemCount > 1)
            {
                std::vector<BYTE> values(dwBufferSize);
                PPDH_FMT_COUNTERVALUE_ITEM data = reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM>(values.data());
                PDH_STATUS status = PdhGetFormattedCounterArray(m_hCounter, m_dwFormat, &dwBufferSize, &dwItemCount, data);

                if (status != PDH_CSTATUS_INVALID_DATA)
                    switch (m_dwFormat)
                    {
                    case PDH_FMT_DOUBLE: Value.doubleValue = std::accumulate(data, data + dwItemCount, 0.0, [](double v, const PDH_FMT_COUNTERVALUE_ITEM& item) { return v + item.FmtValue.doubleValue; }); break;
                    case PDH_FMT_LONG:   Value.longValue = std::accumulate(data, data + dwItemCount, LONG(0), [](LONG v, const PDH_FMT_COUNTERVALUE_ITEM& item) { return v + item.FmtValue.longValue; }); break;
                    }
            }

            const std::wstring text = FormatPdhValue(m_dwFormat, dwType, Value, m_szName, m_szUnit);
            if (SetText(text.c_str()))
                FixSize();
        }
        break;
        }
    }

private:
    std::unique_ptr<PDH_HQUERY, QueryDeleter> m_hQuery;
    PDH_HCOUNTER m_hCounter = NULL;
    DWORD m_dwFormat = PDH_FMT_DOUBLE;
    TCHAR m_szName[100] = TEXT("");
    TCHAR m_szUnit[100] = TEXT("");
};

extern "C"
{
    __declspec(dllexport) HWND CreateWidget(HWND hWndParent, const WidgetParams* params) { return g_hwndMonitor = *PerfWidgetWindow::Create(hWndParent, params); }
}
