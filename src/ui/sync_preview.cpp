#include "sync_preview.h"
#include "../resource.h"
using namespace simple_editor::core;
CSyncPreview::CSyncPreview(SyncBundle bundle,CWnd* owner) : CDialog(IDD_SYNC_PREVIEW,owner),bundle_(std::move(bundle)) {
    for (std::size_t p = 0; p < bundle_.plans.size(); ++p) for (std::size_t i = 0; i < bundle_.plans[p].operations.size(); ++i) rows_.push_back({p,i,false});
    for (std::size_t i = 0; i < bundle_.conflicts.size(); ++i) rows_.push_back({0,i,true});
}
BOOL CSyncPreview::OnInitDialog() {
    CDialog::OnInitDialog(); list_.SubclassDlgItem(IDC_SYNC_OPERATIONS,this); list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    list_.InsertColumn(0,L"Include",LVCFMT_LEFT,65); list_.InsertColumn(1,L"Operation / direction",LVCFMT_LEFT,210); list_.InsertColumn(2,L"Relative path",LVCFMT_LEFT,280); list_.InsertColumn(3,L"Source bytes",LVCFMT_RIGHT,110); list_.InsertColumn(4,L"Target bytes / conflict",LVCFMT_LEFT,300); list_.SetItemCountEx(static_cast<int>(rows_.size()));
    list_.InsertColumn(5,L"Source root",LVCFMT_LEFT,500); list_.InsertColumn(6,L"Target root",LVCFMT_LEFT,500);
    CRect button; GetDlgItem(IDOK)->GetWindowRect(button); ScreenToClient(button); skip_.Create(L"Skip selected / subtree",WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,CRect(12,button.top,205,button.bottom),this,1780); all_.Create(L"Include all",WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,CRect(212,button.top,312,button.bottom),this,1781); skip_.SetFont(GetFont()); all_.SetFont(GetFont());
    std::wstring text = std::to_wstring(rows_.size()) + L" preview entries. Select entries to skip; selecting a directory skips its subtree. Conflicts require exclusion or resolution. Every overwrite/removal retains a backup.\r\n";
    for (std::size_t p = 0; p < bundle_.plans.size(); ++p) text += L"[" + std::to_wstring(p + 1) + L"] " + bundle_.plans[p].source_root.wstring() + L" -> " + bundle_.plans[p].target_root.wstring() + L"\r\n";
    text += bundle_.note; SetDlgItemTextW(IDC_SYNC_SUMMARY,text.c_str()); return TRUE;
}
BOOL CSyncPreview::OnNotify(WPARAM wparam,LPARAM lparam,LRESULT* result) {
    auto* header = reinterpret_cast<NMHDR*>(lparam);
    if (header->idFrom == IDC_SYNC_OPERATIONS && header->code == LVN_GETDISPINFO) {
        auto* info = reinterpret_cast<NMLVDISPINFO*>(header); if ((info->item.mask & LVIF_TEXT) && info->item.iItem >= 0 && static_cast<std::size_t>(info->item.iItem) < rows_.size()) {
            const auto& row = rows_[info->item.iItem]; std::wstring value;
            if (info->item.iSubItem == 0) value = row.skip ? L"SKIP" : L"Yes";
            else if (row.conflict) { const auto& item = bundle_.conflicts[row.index]; value = info->item.iSubItem == 1 ? L"CONFLICT" : info->item.iSubItem == 2 ? item.relative.wstring() : info->item.iSubItem == 4 ? item.reason : L""; }
            else { const auto& plan = bundle_.plans[row.plan]; const auto& item = plan.operations[row.index]; if (info->item.iSubItem == 1) value = item.action == SyncAction::Copy ? L"Copy / update" : item.action == SyncAction::CreateDirectory ? L"Create folder" : L"Move to backup"; else if (info->item.iSubItem == 2) value = item.relative.wstring(); else value = std::to_wstring(info->item.iSubItem == 3 ? item.source.size : item.target.size); }
            if (!row.conflict && info->item.iSubItem == 1) value = L"[" + std::to_wstring(row.plan + 1) + L"] " + value;
            if (!row.conflict && info->item.iSubItem >= 5) value = (info->item.iSubItem == 5 ? bundle_.plans[row.plan].source_root : bundle_.plans[row.plan].target_root).wstring();
            wcsncpy_s(info->item.pszText,info->item.cchTextMax,value.c_str(),_TRUNCATE);
        } *result = 0; return TRUE;
    }
    return CDialog::OnNotify(wparam,lparam,result);
}
void CSyncPreview::SkipSelected() {
    const auto relative = [&](const Row& row) { return row.conflict ? bundle_.conflicts[row.index].relative : bundle_.plans[row.plan].operations[row.index].relative; };
    for (int selected = list_.GetNextItem(-1,LVNI_SELECTED); selected >= 0; selected = list_.GetNextItem(selected,LVNI_SELECTED)) {
        if (static_cast<std::size_t>(selected) >= rows_.size()) continue; const auto chosen = relative(rows_[selected]);
        for (auto& row : rows_) { auto path = relative(row); while (!path.empty()) { if (CompareStringOrdinal(path.c_str(),-1,chosen.c_str(),-1,TRUE) == CSTR_EQUAL) { row.skip = true; break; } path = path.parent_path(); } }
    } list_.Invalidate();
}
BOOL CSyncPreview::OnCommand(WPARAM wparam,LPARAM lparam) {
    if (LOWORD(wparam) == 1780) { SkipSelected(); return TRUE; }
    if (LOWORD(wparam) == 1781) { for (auto& row : rows_) row.skip = false; list_.Invalidate(); return TRUE; }
    return CDialog::OnCommand(wparam,lparam);
}
SyncBundle CSyncPreview::Selected() const {
    SyncBundle result; result.note = bundle_.note; result.plans = bundle_.plans; for (auto& plan : result.plans) plan.operations.clear();
    for (const auto& row : rows_) if (!row.skip) { if (row.conflict) result.conflicts.push_back(bundle_.conflicts[row.index]); else result.plans[row.plan].operations.push_back(bundle_.plans[row.plan].operations[row.index]); }
    return result;
}
void CSyncPreview::OnOK() { if (!Selected().conflicts.empty()) { AfxMessageBox(L"Resolve the conflicts or select and skip them before applying other operations.",MB_ICONWARNING); return; } CDialog::OnOK(); }
