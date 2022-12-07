#pragma once
#define NOMINMAX
#include <Windows.h>
#include <memory>
#include "markable.h"

class ReleaseDCDeleter
{
public:
    ReleaseDCDeleter(HWND hWnd)
        : hWnd(hWnd)
    {
    }

    typedef HDC pointer;

    void operator()(pointer hDC) const noexcept
    {
        ReleaseDC(hWnd, hDC);
    }

private:
    HWND hWnd;
};

_NODISCARD inline auto MakeGetDC(const HWND hWnd)
{
    //HDC hDC = GetDC(hWnd);
    return std::unique_ptr<HDC, ReleaseDCDeleter>(GetDC(hWnd), ReleaseDCDeleter(hWnd));
}

template <class T, BOOL(WINAPI *F)(T t)>
class HandleDeleter
{
public:
    typedef T pointer;

    void operator()(pointer h) const noexcept
    {
        F(h);
    }
};

template <class T, class U, BOOL (WINAPI *F)(U t)>
class HandleDeleter2
{
public:
    typedef T pointer;

    void operator()(pointer h) const noexcept
    {
        F(h);
    }
};

typedef std::unique_ptr<HFONT, HandleDeleter2<HFONT, HGDIOBJ, DeleteObject>> FontPtr;
typedef std::unique_ptr<HMENU, HandleDeleter<HMENU, DestroyMenu>> MenuPtr;

template <class T>
class FakePtr : public ak_toolkit::markable_ns::markable<T>
{
public:
    using ak_toolkit::markable_ns::markable<T>::markable;

    operator bool() const
    {
        return this->has_value();
    }

    FakePtr& operator=(nullptr_t)
    {
        *this = FakePtr();
        return *this;
    }
};

template <class T, class P, typename P::value_type(WINAPI *F)(T t, typename P::value_type a)>
class GdiFunctionDeleter
{
public:
    GdiFunctionDeleter(T t)
        : t(t)
    {
    }

    typedef FakePtr<P> pointer;

    void operator()(pointer mode) const noexcept
    {
        F(t, mode.value());
    }

    static pointer call(T t, typename P::value_type a)
    {
        return pointer(F(t, a));
    }

private:
    T t;
};

_NODISCARD inline auto MakeSelectObject(const HDC hDC, const HGDIOBJ o)
{
    typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_int<HGDIOBJ, INVALID_HANDLE_VALUE>, SelectObject> Deleter;
    return std::unique_ptr<HGDIOBJ, Deleter>(Deleter::call(hDC, o), Deleter(hDC));
}

_NODISCARD inline auto MakeBkMode(const HDC hDC, const int mode)
{
    typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_int<int, -1>, SetBkMode> Deleter;
    return std::unique_ptr<HGDIOBJ, Deleter>(Deleter::call(hDC, mode), Deleter(hDC));
}

_NODISCARD inline auto MakeBkColor(const HDC hDC, const COLORREF c)
{
    typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_int<COLORREF, RGB(255, 0, 255)>, SetBkColor> Deleter;
    //typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_optional<std::optional<COLORREF>>, SetBkColor> Deleter; // TODO This is needed because all values are valid
    return std::unique_ptr<COLORREF, Deleter>(Deleter::call(hDC, c), Deleter(hDC));
}

_NODISCARD inline auto MakeTextColor(const HDC hDC, const COLORREF c)
{
    typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_int<COLORREF, RGB(255, 0, 255)>, SetTextColor> Deleter;
    //typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_optional<std::optional<COLORREF>>, SetTextColor> Deleter; // TODO This is needed because all values are valid
    return std::unique_ptr<COLORREF, Deleter>(Deleter::call(hDC, c), Deleter(hDC));
}

_NODISCARD inline auto MakeDCBrushColor(const HDC hDC, const COLORREF c)
{
    typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_int<COLORREF, RGB(255, 0, 255)>, SetDCBrushColor> Deleter;
    //typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_optional<std::optional<COLORREF>>, SetDCBrushColor> Deleter; // TODO This is needed because all values are valid
    return std::unique_ptr<COLORREF, Deleter>(Deleter::call(hDC, c), Deleter(hDC));
}

_NODISCARD inline auto MakeDCPenColor(const HDC hDC, const COLORREF c)
{
    typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_int<COLORREF, RGB(255, 0, 255)>, SetDCPenColor> Deleter;
    //typedef GdiFunctionDeleter<HDC, ak_toolkit::mark_optional<std::optional<COLORREF>>, SetDCPenColor> Deleter; // TODO This is needed because all values are valid
    return std::unique_ptr<COLORREF, Deleter>(Deleter::call(hDC, c), Deleter(hDC));
}


inline bool operator !=(SIZE sz1, SIZE sz2)
{
    return sz1.cx != sz2.cx || sz1.cy != sz2.cy;
}

inline LONG Width(RECT rc)
{
    return rc.right - rc.left;
}

inline LONG Height(RECT rc)
{
    return rc.bottom - rc.top;
}

inline RECT Rect(POINT p, SIZE sz)
{
    return { p.x, p.y, p.x + sz.cx, p.y + sz.cy };
}

inline SIZE Size(RECT rc)
{
    return { rc.right - rc.left, rc.bottom - rc.top };
}


inline BOOL Rectangle(HDC hDC, RECT rc)
{
    return Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
}

inline BOOL RoundRect(HDC hDC, RECT rc, int w, int h)
{
    return RoundRect(hDC, rc.left, rc.top, rc.right, rc.bottom, w, h);
}
