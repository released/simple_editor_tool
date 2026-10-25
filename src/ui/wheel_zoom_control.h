#pragma once

#include <afxwin.h>
#include <functional>

template<class Control>
class CWheelZoomControl : public Control {
public:
    std::function<void(short)> wheel_zoom;

protected:
    LRESULT WindowProc(UINT message, WPARAM wparam, LPARAM lparam) override {
        if (message == WM_MOUSEWHEEL && wheel_zoom) {
            const UINT flags = LOWORD(wparam);
            if (flags & MK_CONTROL) {
                wheel_zoom(static_cast<short>(HIWORD(wparam)));
                return 0;
            }
            // Shift requests the control's ordinary vertical scrolling.
            wparam = MAKEWPARAM(flags & ~MK_SHIFT, HIWORD(wparam));
        }
        return Control::WindowProc(message, wparam, lparam);
    }
};
