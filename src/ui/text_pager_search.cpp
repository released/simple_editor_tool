#include "text_pager.h"
#include "editor_dialogs.h"
using namespace simple_editor::core;
void CTextPager::FindDialog() {
    if (job_.valid()) return;
    CValueDialog dialog(this,L"Find in large text",L"Exact literal text (UTF-8/UTF-16, up to 4096 characters). Search starts at the current page; F3 continues to end of file.",query_);
    dialog.validate = [](const auto& text,std::wstring* error) { return !text.empty() && text.size() <= 4096 && ValidateText(text,error); };
    if (dialog.DoModal() == IDOK) { query_ = dialog.value; next_match_ = page_.begin; FindNext(); }
}
bool CTextPager::FindNext() {
    if (job_.valid() || query_.empty()) return false;
    const auto query = query_; const auto codec = page_.codec; const auto start = next_match_; cancelled_ = false;
    try { job_ = std::async(std::launch::async,[this,query,codec,start] { Result result; result.searching = true; SaveControl control; control.cancelled = &cancelled_; if (!FindPagedText(*source_,codec,query,start,control,&result.found,&result.after,&result.error) && result.error.empty()) result.error = L"Paged search failed or was cancelled."; return result; }); }
    catch (...) { SetDlgItemTextW(2301,L"Could not start paged search."); return false; }
    for (int id : {2302,2303,2305,2306,2307,2308}) GetDlgItem(id)->EnableWindow(FALSE); SetDlgItemTextW(2301,L"Searching the file without loading it all. Esc cancels."); return true;
}
