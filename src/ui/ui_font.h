#pragma once
#include <afxwin.h>
#include <initializer_list>

namespace simple_editor::ui {
inline bool CreateUiFont(CFont& font, CWnd& owner) {
    LOGFONT spec{};
    CClientDC dc(&owner);
    spec.lfHeight = -MulDiv(9, dc.GetDeviceCaps(LOGPIXELSY), 72);
    spec.lfWeight = FW_NORMAL;
    spec.lfCharSet = DEFAULT_CHARSET;
    spec.lfQuality = CLEARTYPE_QUALITY;
    spec.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    wcscpy_s(spec.lfFaceName, L"Segoe UI");
    return font.CreateFontIndirect(&spec) != FALSE;
}
inline void BindUiFont(CWnd& owner, CFont& font, std::initializer_list<int> ids) {
    for (const auto id : ids) {
        auto* control = owner.GetDlgItem(id);
        if (control && control->GetSafeHwnd()) control->SetFont(&font, FALSE);
    }
}
}
