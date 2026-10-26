#include "compare_presets.h"
#include "line_merge.h"
#include "json_rules.h"
#include "table_compare.h"
#include "folder_text_rules.h"
#include "compare_rules.h"
#include "hex_bookmarks.h"
#include <map>
#include <set>
namespace simple_editor::core {
WorkspaceEntry PresetSettings(const WorkspaceEntry& s) {
    WorkspaceEntry e; e.comparison = true; e.mode = s.mode; e.options = s.options; e.advanced_encoding = s.advanced_encoding; e.advanced_encoding_right = s.advanced_encoding_right; e.advanced_encoding_base = s.advanced_encoding_base;
    e.include = s.include; e.exclude = s.exclude; e.table_rules = s.table_rules; e.text_rules = s.text_rules; e.json_rules = s.json_rules; e.folder_text_rules = s.folder_text_rules; e.context_lines = s.context_lines; e.folder_tree = s.folder_tree;
    e.folder_criterion = s.folder_criterion; e.folder_tolerance = s.folder_tolerance; e.folder_filter = s.folder_filter; e.folder_sort = s.folder_sort; e.folder_sort_descending = s.folder_sort_descending;
    return e;
}
namespace {
bool Valid(const ComparePreset& p,std::wstring* error) {
    const auto& e = p.settings;
    TableCompareOptions table; TextCompareRules text; FolderTextSettings folder; JsonCompareOptions json;
    if (p.name.empty() || p.name.size() > 80 || p.name.find_first_of(L"\r\n\t") != std::wstring::npos || p.name.find(L'\0') != std::wstring::npos || e.mode < 0 || e.mode > 7 || e.advanced_encoding < 0 || e.advanced_encoding > 5 || e.advanced_encoding_right < -1 || e.advanced_encoding_right > 5 || e.advanced_encoding_base < -1 || e.advanced_encoding_base > 5 || e.options > 1023 || e.context_lines > 50 || e.folder_criterion > 3 || e.folder_tolerance > 2 || e.folder_filter > 11 || e.folder_sort > 6 || e.include.size() > 4096 || e.exclude.size() > 4096) { if (error) *error = L"Invalid comparison preset name/settings."; return false; }
    return ParseTableOptions(e.table_rules,&table,error) && ParseTextRules(e.text_rules,&text,error) && ParseFolderTextSettings(e.folder_text_rules,&folder,error) && ParseJsonRules(e.json_rules,&json,error);
}
}
bool SaveComparePresets(const std::filesystem::path& path,const std::vector<ComparePreset>& presets,std::wstring* error) {
    if (presets.size() > 32) { if (error) *error = L"At most 32 named presets may be saved."; return false; }
    std::wstring text = L"[SimpleEditorToolComparisonPresets]\r\nVersion=" + EncodeSetting(L"1") + L"\r\nCount=" + EncodeSetting(std::to_wstring(presets.size())) + L"\r\n"; std::set<std::wstring> names;
    for (std::size_t i = 0; i < presets.size(); ++i) { const auto& p = presets[i]; if (!Valid(p,error) || !names.insert(p.name).second) return false; const auto e = PresetSettings(p.settings); text += L"\r\n[Preset" + std::to_wstring(i) + L"]\r\n";
        const auto field = [&](const wchar_t* key,const std::wstring& value) { text += key; text += L"=" + EncodeSetting(value) + L"\r\n"; };
        field(L"Name",p.name); field(L"Include",e.include); field(L"Exclude",e.exclude); field(L"TableRules",e.table_rules); field(L"TextRules",e.text_rules); field(L"JsonRules",e.json_rules); field(L"FolderTextRules",e.folder_text_rules);
        const auto number = [&](const wchar_t* key,unsigned value) { field(key,std::to_wstring(value)); };
        number(L"Mode",e.mode); number(L"Options",e.options); number(L"Encoding",e.advanced_encoding); number(L"RightEncoding",e.advanced_encoding_right < 0 ? e.advanced_encoding : e.advanced_encoding_right); number(L"BaseEncoding",e.advanced_encoding_base < 0 ? e.advanced_encoding : e.advanced_encoding_base); number(L"Context",e.context_lines); number(L"FolderTree",e.folder_tree); number(L"Criterion",e.folder_criterion); number(L"Tolerance",e.folder_tolerance); number(L"Filter",e.folder_filter); number(L"Sort",e.folder_sort); number(L"Descending",e.folder_sort_descending);
    }
    if (text.size() > 500000) { if (error) *error = L"Preset file exceeds 1 MiB; reduce settings/preset count."; return false; }
    std::vector<std::uint8_t> bytes; TextCodec codec; codec.encoding = TextEncoding::Utf16Le; codec.bom = true; return EncodeText(text,codec,true,&bytes,error) && WriteBytesSafely(path,bytes,error);
}
bool LoadComparePresets(const std::filesystem::path& path,std::vector<ComparePreset>* output,std::wstring* error) {
    FileStamp stamp; if (!InspectFile(path,&stamp,error)) return false; if (!stamp.exists) { output->clear(); return true; }
    BinaryDocument document; std::vector<std::uint8_t> bytes; std::wstring text; TextCodec codec;
    if (!document.Open(path.wstring(),error) || document.Size() > 1024 * 1024 || !document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) || !DecodeText(bytes,TextEncoding::Auto,&text,&codec,error) || !document.ValidateSource(true,error)) return false;
    std::map<std::wstring,std::wstring> fields; std::wstring section;
    const auto fail = [&]() { if (error) *error = L"Malformed, oversized or duplicate comparison preset setting."; return false; };
    for (const auto& line : SplitLines(text)) { if (line.text.empty()) continue; if (line.text.front() == L'[' && line.text.back() == L']') { section = line.text.substr(1,line.text.size() - 2); continue; } const auto equal = line.text.find(L'='); std::wstring value; if (equal == std::wstring::npos || !DecodeSetting(line.text.substr(equal + 1),&value) || !fields.emplace(section + L"/" + line.text.substr(0,equal),value).second) return fail(); }
    bool valid = true;
    const auto number = [&](const std::wstring& key,unsigned maximum) { const auto& value = fields[key]; if (value.empty() || value.find_first_not_of(L"0123456789") != std::wstring::npos) { valid = false; return 0u; } try { const auto n = std::stoull(value); if (n > maximum) valid = false; return static_cast<unsigned>(n); } catch (...) { valid = false; return 0u; } };
    if (number(L"SimpleEditorToolComparisonPresets/Version",1) != 1) return fail(); const auto count = number(L"SimpleEditorToolComparisonPresets/Count",32); if (!valid) return fail();
    std::vector<ComparePreset> result; std::set<std::wstring> names;
    for (unsigned i = 0; i < count; ++i) { const auto prefix = L"Preset" + std::to_wstring(i) + L"/"; ComparePreset p; auto& e = p.settings; e.comparison = true; p.name = fields[prefix + L"Name"]; e.include = fields[prefix + L"Include"]; e.exclude = fields[prefix + L"Exclude"]; e.table_rules = fields[prefix + L"TableRules"]; e.text_rules = fields[prefix + L"TextRules"]; e.json_rules = fields[prefix + L"JsonRules"]; e.folder_text_rules = fields[prefix + L"FolderTextRules"];
        e.mode = number(prefix + L"Mode",7); e.options = number(prefix + L"Options",1023); e.advanced_encoding = number(prefix + L"Encoding",5); e.advanced_encoding_right = fields.count(prefix + L"RightEncoding") ? number(prefix + L"RightEncoding",5) : e.advanced_encoding; e.advanced_encoding_base = fields.count(prefix + L"BaseEncoding") ? number(prefix + L"BaseEncoding",5) : e.advanced_encoding; e.context_lines = number(prefix + L"Context",50); e.folder_tree = number(prefix + L"FolderTree",1) != 0; e.folder_criterion = number(prefix + L"Criterion",3); e.folder_tolerance = number(prefix + L"Tolerance",2); e.folder_filter = number(prefix + L"Filter",11); e.folder_sort = number(prefix + L"Sort",6); e.folder_sort_descending = number(prefix + L"Descending",1) != 0;
        if (!valid || !Valid(p,error) || !names.insert(p.name).second) return fail(); result.push_back(std::move(p));
    }
    *output = std::move(result); return true;
}
}
