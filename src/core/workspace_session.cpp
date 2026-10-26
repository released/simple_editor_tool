#include "workspace_session.h"
#include "line_merge.h"
#include "compare_region.h"
#include "hex_bookmarks.h"
#include "syntax_profiles.h"
#include <map>

namespace simple_editor::core {
bool SaveWorkspaceFile(const std::filesystem::path& path, const std::vector<WorkspaceEntry>& entries, std::wstring* error) {
    if (entries.size() > 200) { if (error) *error = L"A workspace file supports up to 200 sessions."; return false; }
    std::wstring text = L"[SimpleEditorToolWorkspace]\r\nVersion=1\r\nCount=" + std::to_wstring(entries.size()) + L"\r\n";
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i]; text += L"\r\n[Session" + std::to_wstring(i) + L"]\r\n";
        if (e.folder_criterion > 3 || e.folder_tolerance > 86400 || e.folder_filter > 11 || e.folder_sort > 6 || e.mode < 0 || e.mode > 7) { if (error) *error = L"Invalid comparison mode or folder view setting."; return false; }
        CompareRegion region; if (!e.binary_region.empty() && !ParseCompareRegionSpec(e.binary_region,&region,error)) return false;
        for (int j = 0; j < 3; ++j) if (e.hex_decoding[j] > 4 || (e.hex_rows[j] != 8 && e.hex_rows[j] != 16 && e.hex_rows[j] != 32) || (e.hex_groups[j] != 1 && e.hex_groups[j] != 2 && e.hex_groups[j] != 4 && e.hex_groups[j] != 8)) { if (error) *error = L"Invalid Hex view layout."; return false; }
        for (const auto* value : {&e.left,&e.right,&e.base,&e.include,&e.exclude,&e.binary_region,&e.sync_baseline}) if (value->find_first_of(L"\r\n") != std::wstring::npos || value->find(L'\0') != std::wstring::npos) { if (error) *error = L"Workspace values must be single-line paths/settings."; return false; }
        const auto field = [&](const wchar_t* key, const std::wstring& value) { text += key; text += L"="; text += value; text += L"\r\n"; };
        field(L"Comparison",std::to_wstring(e.comparison)); field(L"Left",e.left); field(L"Right",e.right); field(L"Base",e.base);
        field(L"Active",std::to_wstring(e.active));
        if (!ValidSyntaxMode(e.syntax_mode)) return false; field(L"SyntaxMode",e.syntax_mode);
        if (e.context_lines > 50 || e.folder_text_rules.size() > 65536) return false; field(L"FolderTextRules",EncodeSetting(e.folder_text_rules)); field(L"ContextLines",std::to_wstring(e.context_lines)); field(L"FolderTree",std::to_wstring(e.folder_tree));
        if (e.hex_bookmarks.size() > 720000 || e.table_rules.size() > 65536 || e.text_rules.size() > 65536 || e.json_rules.size() > 65536 || e.manual_anchors.size() > 4096) { if (error) *error = L"Feature settings exceed workspace limits."; return false; }
        field(L"JsonRules",EncodeSetting(e.json_rules));
        if (!e.hex_bookmarks.empty()) { HexBookmarks bookmarks; if (!bookmarks.Load(e.hex_bookmarks,UINT64_MAX,error)) return false; }
        if (e.crc_profile.size() > 4096) return false; field(L"CrcProfile",EncodeSetting(e.crc_profile));
        field(L"HexBookmarks",e.hex_bookmarks); field(L"TableRules",EncodeSetting(e.table_rules)); field(L"TextRules",EncodeSetting(e.text_rules)); field(L"ManualAnchors",EncodeSetting(e.manual_anchors)); field(L"DualView",std::to_wstring(e.dual_view));
        if (e.advanced_encoding < 0 || e.advanced_encoding > 5 || e.advanced_encoding_right < -1 || e.advanced_encoding_right > 5 || e.advanced_encoding_base < -1 || e.advanced_encoding_base > 5) return false;
        field(L"Mode",std::to_wstring(e.mode)); field(L"Page",std::to_wstring(e.page)); field(L"Encoding",std::to_wstring(e.encoding)); field(L"AdvancedEncoding",std::to_wstring(e.advanced_encoding));
        field(L"RightEncoding",std::to_wstring(e.advanced_encoding_right < 0 ? e.advanced_encoding : e.advanced_encoding_right)); field(L"BaseEncoding",std::to_wstring(e.advanced_encoding_base < 0 ? e.advanced_encoding : e.advanced_encoding_base));
        for (int j = 0; j < 3; ++j) field((L"Zoom" + std::to_wstring(j)).c_str(),std::to_wstring(e.zoom[j]));
        field(L"BinaryRegion",e.binary_region); for (int j = 0; j < 3; ++j) { const auto suffix = std::to_wstring(j); field((L"HexDecoding" + suffix).c_str(),std::to_wstring(e.hex_decoding[j])); field((L"HexRows" + suffix).c_str(),std::to_wstring(e.hex_rows[j])); field((L"HexGroup" + suffix).c_str(),std::to_wstring(e.hex_groups[j])); field((L"HexBase" + suffix).c_str(),std::to_wstring(e.hex_bases[j])); }
        field(L"Options",std::to_wstring(e.options)); field(L"Include",e.include); field(L"Exclude",e.exclude); field(L"Caret",std::to_wstring(e.caret)); field(L"TopRow",std::to_wstring(e.top_row));
        field(L"FolderCriterion",std::to_wstring(e.folder_criterion)); field(L"FolderTolerance",std::to_wstring(e.folder_tolerance)); field(L"FolderFilter",std::to_wstring(e.folder_filter)); field(L"FolderSort",std::to_wstring(e.folder_sort)); field(L"FolderSortDescending",std::to_wstring(e.folder_sort_descending));
        if (e.sync_policy > 3) { if (error) *error = L"Invalid sync policy."; return false; }
        field(L"SyncPolicy",std::to_wstring(e.sync_policy)); field(L"SyncDeletions",std::to_wstring(e.sync_deletions)); field(L"SyncBaseline",e.sync_baseline);
    }
    if (text.size() > 500000) { if (error) *error = L"Workspace settings exceed the 1 MiB limit."; return false; }
    std::vector<std::uint8_t> bytes; TextCodec codec; codec.encoding = TextEncoding::Utf16Le; codec.bom = true;
    return EncodeText(text,codec,true,&bytes,error) && WriteBytesSafely(path,bytes,error);
}
bool LoadWorkspaceFile(const std::filesystem::path& path, std::vector<WorkspaceEntry>* entries, std::wstring* error) {
    BinaryDocument document; std::vector<std::uint8_t> bytes; std::wstring text; TextCodec codec;
    if (!document.Open(path.wstring(),error) || document.Size() > 1024 * 1024 || !document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) || !DecodeText(bytes,TextEncoding::Auto,&text,&codec,error)) { if (error && error->empty()) *error = L"Invalid or oversized workspace settings."; return false; }
    std::map<std::wstring,std::wstring> fields; std::wstring section;
    for (const auto& line : SplitLines(text)) {
        if (line.text.empty() || line.text[0] == L';') continue;
        if (line.text.front() == L'[' && line.text.back() == L']') { section = line.text.substr(1,line.text.size() - 2); continue; }
        const auto equal = line.text.find(L'='); if (equal == std::wstring::npos) { if (error) *error = L"Malformed workspace field."; return false; }
        const auto key = section + L"/" + line.text.substr(0,equal);
        if (!fields.emplace(key,line.text.substr(equal + 1)).second) { if (error) *error = L"Duplicate workspace field."; return false; }
    }
    const auto number = [&](const std::wstring& key, std::uint64_t fallback, std::uint64_t maximum, bool* valid) {
        const auto found = fields.find(key); if (found == fields.end()) return fallback;
        const auto& value = found->second; if (value.empty() || value.find_first_not_of(L"0123456789") != std::wstring::npos) { *valid = false; return fallback; }
        try { std::size_t used = 0; const auto result = std::stoull(value,&used); if (used != value.size() || result > maximum) *valid = false; return static_cast<std::uint64_t>(result); }
        catch (...) { *valid = false; return fallback; }
    };
    bool valid = true;
    if (number(L"SimpleEditorToolWorkspace/Version",0,1,&valid) != 1) valid = false;
    const auto count = number(L"SimpleEditorToolWorkspace/Count",201,200,&valid);
    if (!valid || count > 200) { if (error) *error = L"Unsupported workspace version or session count."; return false; }
    std::vector<WorkspaceEntry> result;
    for (std::size_t i = 0; i < count; ++i) {
        const auto prefix = L"Session" + std::to_wstring(i) + L"/"; WorkspaceEntry e;
        e.comparison = number(prefix + L"Comparison",0,1,&valid) != 0;
        e.active = number(prefix + L"Active",0,1,&valid) != 0;
        e.syntax_mode = fields[prefix + L"SyntaxMode"]; if (!ValidSyntaxMode(e.syntax_mode)) valid = false;
        e.context_lines = static_cast<unsigned>(number(prefix + L"ContextLines",0,50,&valid)); e.folder_tree = number(prefix + L"FolderTree",0,1,&valid) != 0; if (!DecodeSetting(fields[prefix + L"FolderTextRules"],&e.folder_text_rules)) valid = false;
        e.hex_bookmarks = fields[prefix + L"HexBookmarks"]; e.dual_view = number(prefix + L"DualView",0,1,&valid) != 0;
        if (!DecodeSetting(fields[prefix + L"CrcProfile"],&e.crc_profile,4096) || !DecodeSetting(fields[prefix + L"JsonRules"],&e.json_rules)) valid = false;
        if (!DecodeSetting(fields[prefix + L"TableRules"],&e.table_rules) || !DecodeSetting(fields[prefix + L"TextRules"],&e.text_rules) || !DecodeSetting(fields[prefix + L"ManualAnchors"],&e.manual_anchors,4096)) valid = false;
        if (!e.hex_bookmarks.empty()) { HexBookmarks bookmarks; if (!bookmarks.Load(e.hex_bookmarks,UINT64_MAX,error)) valid = false; }
        e.left = fields[prefix + L"Left"]; e.right = fields[prefix + L"Right"]; e.base = fields[prefix + L"Base"]; e.include = fields.count(prefix + L"Include") ? fields[prefix + L"Include"] : L"*"; e.exclude = fields[prefix + L"Exclude"];
        e.binary_region = fields[prefix + L"BinaryRegion"]; CompareRegion region; if (!e.binary_region.empty() && !ParseCompareRegionSpec(e.binary_region,&region,error)) valid = false;
        for (const auto* value : {&e.left,&e.right,&e.base,&e.include,&e.exclude,&e.binary_region}) if (value->find(L'\0') != std::wstring::npos) valid = false;
        e.mode = static_cast<int>(number(prefix + L"Mode",0,7,&valid)); e.page = static_cast<int>(number(prefix + L"Page",0,2,&valid));
        e.encoding = static_cast<int>(number(prefix + L"Encoding",0,5,&valid)); e.advanced_encoding = static_cast<int>(number(prefix + L"AdvancedEncoding",0,5,&valid));
        e.advanced_encoding_right = static_cast<int>(number(prefix + L"RightEncoding",e.advanced_encoding,5,&valid)); e.advanced_encoding_base = static_cast<int>(number(prefix + L"BaseEncoding",e.advanced_encoding,5,&valid));
        e.options = static_cast<unsigned>(number(prefix + L"Options",88,1023,&valid)); e.caret = number(prefix + L"Caret",0,UINT64_MAX,&valid); e.top_row = number(prefix + L"TopRow",0,UINT64_MAX,&valid);
        e.folder_criterion = static_cast<unsigned>(number(prefix + L"FolderCriterion",0,3,&valid)); e.folder_tolerance = static_cast<unsigned>(number(prefix + L"FolderTolerance",0,86400,&valid)); e.folder_filter = static_cast<unsigned>(number(prefix + L"FolderFilter",0,11,&valid)); e.folder_sort = static_cast<unsigned>(number(prefix + L"FolderSort",0,6,&valid)); e.folder_sort_descending = number(prefix + L"FolderSortDescending",0,1,&valid) != 0;
        e.sync_policy = static_cast<unsigned>(number(prefix + L"SyncPolicy",0,3,&valid)); e.sync_deletions = number(prefix + L"SyncDeletions",0,1,&valid) != 0; e.sync_baseline = fields[prefix + L"SyncBaseline"]; if (e.sync_baseline.size() > 32767 || e.sync_baseline.find(L'\0') != std::wstring::npos) valid = false;
        for (int j = 0; j < 3; ++j) { e.zoom[j] = static_cast<int>(number(prefix + L"Zoom" + std::to_wstring(j),100,300,&valid)); if (e.zoom[j] < 50) valid = false; }
        for (int j = 0; j < 3; ++j) { const auto suffix = std::to_wstring(j); e.hex_decoding[j] = static_cast<unsigned>(number(prefix + L"HexDecoding" + suffix,0,4,&valid)); e.hex_rows[j] = static_cast<unsigned>(number(prefix + L"HexRows" + suffix,16,32,&valid)); e.hex_groups[j] = static_cast<unsigned>(number(prefix + L"HexGroup" + suffix,8,8,&valid)); e.hex_bases[j] = number(prefix + L"HexBase" + suffix,0,UINT64_MAX,&valid); if (e.hex_decoding[j] > 4 || (e.hex_rows[j] != 8 && e.hex_rows[j] != 16 && e.hex_rows[j] != 32) || (e.hex_groups[j] != 1 && e.hex_groups[j] != 2 && e.hex_groups[j] != 4 && e.hex_groups[j] != 8)) valid = false; }
        if ((!e.comparison && e.left.empty()) || e.left.size() > 32767 || e.right.size() > 32767 || e.base.size() > 32767) valid = false;
        result.push_back(std::move(e));
    }
    if (!valid) { if (error) *error = L"Invalid workspace path/view setting."; return false; }
    *entries = std::move(result); return true;
}
} // namespace simple_editor::core
