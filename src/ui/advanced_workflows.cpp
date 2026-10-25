#include "advanced_compare.h"
#include "feature_dialogs.h"
#include "../resource.h"
#include "../core/folder_text_rules.h"
#include "../core/search.h"
using namespace simple_editor::core;
void CAdvancedCompareView::WorkflowSettings(UINT command) {
    if (Busy()) return;
    if (command == ID_WORKFLOW_FOLDER_RULES) { if (mode_ != 4) { Status(L"Open a folder comparison first."); return; } CFeatureInput dialog(this,L"Folder text comparison rules",L"enabled=yes/no; include=*.cpp;*.h;*.txt; encoding=auto/utf8/utf16le/utf16be/big5/system; ignore-space/case/eol=yes/no; plus existing literal text rules. Text analysis is independent from raw comparison and synchronization; 16 MiB per input.",folder_text_rules_.empty() ? L"enabled=yes\r\ninclude=*.cpp;*.h;*.txt;*.ini;*.log\r\nignore-eol=yes" : folder_text_rules_); dialog.validate = [](const std::wstring& text,std::wstring* error) { FolderTextSettings settings; return ParseFolderTextSettings(text,&settings,error); }; modal_ = true; const auto answer = dialog.DoModal(); modal_ = false; if (answer == IDOK) { folder_text_rules_ = dialog.value; InvalidateInputs(); if (changed) changed(); } return; }
    if (command == ID_WORKFLOW_CONTEXT) { if (mode_ != 2 && mode_ != 3) { Status(L"Context collapse applies to text comparisons."); return; } CFeatureInput dialog(this,L"Unchanged comparison context",L"Enter 0 to disable collapse, or 1-50 lines around significant differences/conflicts. Double-click a collapsed placeholder to expand it. This changes only the view; merge output, Undo, and reports retain their original mappings.",std::to_wstring(context_lines_)); std::uint64_t count = 0; dialog.validate = [&](const std::wstring& value,std::wstring*) { return ParseAddress(L"d:" + value,&count) && count <= 50; }; modal_ = true; const auto answer = dialog.DoModal(); modal_ = false; if (answer == IDOK) { context_lines_ = static_cast<unsigned>(count); context_expanded_.clear(); RebuildRows(); if (changed) changed(); } return; }
    if (mode_ != 4) { Status(L"Result-tree commands apply to folder comparisons."); return; }
    if (command == ID_WORKFLOW_TREE) folder_tree_ = !folder_tree_;
    else if (command == ID_WORKFLOW_TREE_COLLAPSE) tree_expanded_.clear();
    else if (command == ID_WORKFLOW_TREE_EXPAND && scan_) { for (std::size_t i = 0; i < scan_->rows.size(); ++i) if (scan_->rows[i].left.directory || scan_->rows[i].right.directory) tree_expanded_.insert(i); folder_tree_ = true; }
    RebuildRows(); Status(folder_tree_ ? L"Folder result tree. Double-click a directory to expand/collapse. Sync all still uses every filtered entry; preview lists actual operations." : L"Flat folder results restored."); if (changed) changed();
}
