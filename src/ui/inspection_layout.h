#pragma once
#include <afxwin.h>
#include <algorithm>
#include <vector>
// Keep text/buttons at their original height and let the two content panes absorb resizing.
class CInspectionLayout {
public:
    void Initialize(CDialog& dialog,int first,int second) {
        CRect client; dialog.GetClientRect(client); original_ = client.Size(); first_ = first; second_ = second; items_.clear();
        for (HWND child = ::GetWindow(dialog.GetSafeHwnd(),GW_CHILD); child; child = ::GetWindow(child,GW_HWNDNEXT)) {
            Item item; item.window = child; ::GetWindowRect(child,&item.rect); dialog.ScreenToClient(item.rect); item.id = ::GetDlgCtrlID(child);
            wchar_t name[32]{}; ::GetClassNameW(child,name,32);
            if (_wcsicmp(name,L"ComboBox") == 0) { RECT dropped{}; ::SendMessageW(child,CB_GETDROPPEDCONTROLRECT,0,reinterpret_cast<LPARAM>(&dropped)); item.combo_height = dropped.bottom - dropped.top; }
            if (item.id == first_) first_bottom_ = item.rect.bottom; if (item.id == second_) second_bottom_ = item.rect.bottom; items_.push_back(item);
        }
    }
    void Fit(CDialog& dialog) {
        MONITORINFO monitor{sizeof(monitor)}; if (!GetMonitorInfoW(MonitorFromWindow(dialog.GetSafeHwnd(),MONITOR_DEFAULTTONEAREST),&monitor)) return;
        CRect window; dialog.GetWindowRect(window);
        const int width = (std::min)(window.Width(),static_cast<int>((std::max)(1L,monitor.rcWork.right - monitor.rcWork.left - 16))),height = (std::min)(window.Height(),static_cast<int>((std::max)(1L,monitor.rcWork.bottom - monitor.rcWork.top - 16)));
        const int x = (std::clamp)(window.left,monitor.rcWork.left,monitor.rcWork.right - width),y = (std::clamp)(window.top,monitor.rcWork.top,monitor.rcWork.bottom - height);
        dialog.SetWindowPos(nullptr,x,y,width,height,SWP_NOACTIVATE | SWP_NOZORDER); Apply(dialog);
    }
    void Apply(CDialog& dialog) {
        if (items_.empty() || original_.cx <= 0) return; CRect client; dialog.GetClientRect(client); const int delta = client.Height() - original_.cy,first_delta = delta / 2,second_delta = delta - first_delta;
        for (const auto& item : items_) {
            auto rect = item.rect;
            if (item.id == first_) rect.bottom += first_delta;
            else if (item.id == second_) { rect.OffsetRect(0,first_delta); rect.bottom += second_delta; }
            else if (rect.top >= second_bottom_) rect.OffsetRect(0,delta);
            else if (rect.top >= first_bottom_) rect.OffsetRect(0,first_delta);
            const int left = MulDiv(rect.left,client.Width(),original_.cx),right = MulDiv(rect.right,client.Width(),original_.cx);
            ::MoveWindow(item.window,left,rect.top,(std::max)(1,right - left),item.combo_height ? item.combo_height : (std::max)(24,rect.Height()),FALSE);
        }
        dialog.RedrawWindow(nullptr,nullptr,RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
private:
    struct Item { HWND window = nullptr; CRect rect; int id = 0,combo_height = 0; };
    CSize original_; int first_ = 0,second_ = 0,first_bottom_ = 0,second_bottom_ = 0;
    std::vector<Item> items_;
};
