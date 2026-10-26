#include "advanced_compare.h"
using namespace simple_editor::core;
std::wstring CAdvancedCompareView::FolderPairKey(const std::wstring& left,const std::wstring& right) const {
    if (!scan_ || mode_ != 4 || stale_) return L"";
    for (const auto& row : scan_->rows) if ((!left.empty() && CompareStringOrdinal(left.c_str(),-1,(scan_->left_root / row.relative).c_str(),-1,TRUE) == CSTR_EQUAL) || (!right.empty() && CompareStringOrdinal(right.c_str(),-1,(scan_->right_root / row.relative).c_str(),-1,TRUE) == CSTR_EQUAL)) return row.relative.wstring(); return L"";
}
bool CAdvancedCompareView::AdjacentFolderFile(const std::wstring& relative,int direction,FolderFile* output) const {
    if (!output || Busy() || !scan_ || stale_ || mode_ != 4 || (direction != -1 && direction != 1)) return false;
    std::vector<std::size_t> files; int current = -1;
    for (auto index : visible_) { const auto& row = scan_->rows[index]; if (row.left.directory || row.right.directory || row.state == FolderState::Equal || row.state == FolderState::Error || row.state == FolderState::TypeConflict) continue; if (CompareStringOrdinal(row.relative.c_str(),-1,relative.c_str(),-1,TRUE) == CSTR_EQUAL) current = static_cast<int>(files.size()); files.push_back(index); }
    const auto next = current + direction; if (current < 0 || next < 0 || static_cast<std::size_t>(next) >= files.size()) return false;
    const auto& row = scan_->rows[files[next]]; FolderFile file; file.relative = row.relative.wstring(); file.left = row.left.exists ? (scan_->left_root / row.relative).wstring() : L""; file.right = row.right.exists ? (scan_->right_root / row.relative).wstring() : L"";
    file.mode = MatchFileFilter(row.relative.extension().wstring(),L".txt;.c;.cpp;.h;.hpp;.py;.js;.ts;.cs;.xml;.json;.ini;.md;.csv;.log;.ps1") ? 2 : 0; *output = std::move(file); return true;
}
