#include "portable_preferences.h"
#include "text_codec.h"
#include "line_merge.h"
#include <algorithm>
#include <cwctype>
#include <map>
#include <set>
namespace simple_editor::core {
namespace {
const wchar_t* const kActions[] = {L"NewFile",L"OpenFiles",L"Save",L"Find",L"Replace",L"FindFiles",L"ReopenClosed"};
const std::wstring kPreferences = L"simpleeditortool.preferences",kRecent = L"simpleeditortool.recent",kShortcuts = L"simpleeditortool.shortcuts";
std::wstring Trim(std::wstring text) { const auto first = text.find_first_not_of(L" \t"); return first == std::wstring::npos ? L"" : text.substr(first,text.find_last_not_of(L" \t") - first + 1); }
std::wstring Lower(std::wstring text) { for (auto& c : text) c = static_cast<wchar_t>(towlower(c)); return text; }
bool Read(const std::filesystem::path& path,std::vector<TextLine>* lines,TextCodec* codec,FileStamp* stamp,std::wstring* error) {
    codec->encoding = TextEncoding::Utf16Le; codec->bom = true;
    if (!InspectFile(path,stamp,error)) return false; if (!stamp->exists) return true;
    BinaryDocument document; std::vector<std::uint8_t> bytes; std::wstring text;
    if (stamp->directory || !document.Open(path.wstring(),error) || document.Size() > 1024 * 1024 || !document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) || !DecodeText(bytes,TextEncoding::Auto,&text,codec,error) || !document.ValidateSource(true,error)) { if (error && error->empty()) *error = L"Invalid or oversized portable INI."; return false; }
    *lines = SplitLines(text); return true;
}
bool Number(const std::wstring& value,unsigned maximum,unsigned* output) { if (value.empty() || value.find_first_not_of(L"0123456789") != std::wstring::npos) return false; try { const auto result = std::stoull(value); if (result > maximum) return false; *output = static_cast<unsigned>(result); return true; } catch (...) { return false; } }
using Fields = std::map<std::wstring,std::wstring>;
Fields Extract(const std::vector<TextLine>& lines) { Fields fields; std::wstring section; for (const auto& line : lines) { const auto text = Trim(line.text); if (text.empty() || text[0] == L';' || text[0] == L'#') continue; if (text.front() == L'[' && text.back() == L']') section = Lower(Trim(text.substr(1,text.size() - 2))); else { const auto at = text.find(L'='); if (at != std::wstring::npos) fields[section + L"/" + Lower(Trim(text.substr(0,at)))] = Trim(text.substr(at + 1)); } } return fields; }
}
bool ParseShortcut(const std::wstring& value,Shortcut* shortcut,std::wstring* error) {
    Shortcut result; auto text = Lower(Trim(value)); if (text.empty() || text == L"none") { *shortcut = result; return true; }
    for (;;) { const auto separator = text.find(L'+'); if (separator == std::wstring::npos) break; const auto modifier = Trim(text.substr(0,separator)); const unsigned flag = modifier == L"ctrl" ? 1 : modifier == L"shift" ? 2 : modifier == L"alt" ? 4 : 0; if (!flag || (result.modifiers & flag)) { if (error) *error = L"Use distinct Ctrl, Shift or Alt modifiers."; return false; } result.modifiers |= flag; text = Trim(text.substr(separator + 1)); }
    if (text.size() == 1 && ((text[0] >= L'a' && text[0] <= L'z') || (text[0] >= L'0' && text[0] <= L'9'))) result.key = static_cast<unsigned>(towupper(text[0]));
    else if (text.size() >= 2 && text[0] == L'f') { unsigned number = 0; if (Number(text.substr(1),24,&number) && number) result.key = VK_F1 + number - 1; }
    if (!result.key || (result.key < VK_F1 && !(result.modifiers & 5))) { if (error) *error = L"Use Ctrl/Alt plus a letter/digit, or F1-F24."; return false; }
    *shortcut = result; return true;
}
std::wstring FormatShortcut(const Shortcut& shortcut) { if (!shortcut.key) return L"None"; std::wstring text; if (shortcut.modifiers & 1) text += L"Ctrl+"; if (shortcut.modifiers & 2) text += L"Shift+"; if (shortcut.modifiers & 4) text += L"Alt+"; text += shortcut.key >= VK_F1 && shortcut.key <= VK_F24 ? L"F" + std::to_wstring(shortcut.key - VK_F1 + 1) : std::wstring(1,static_cast<wchar_t>(shortcut.key)); return text; }
bool ValidatePreferences(const AppPreferences& p,std::wstring* error) {
    if (p.font.empty() || p.font.size() > 31 || p.font.find_first_of(L"\r\n") != std::wstring::npos || p.font.find(L'\0') != std::wstring::npos || p.font_tenths < 60 || p.font_tenths > 320 || p.theme > 2 || !p.tab_width || p.tab_width > 16 || p.snapshot_seconds < 10 || p.snapshot_seconds > 600 || p.recent.size() > 20) { if (error) *error = L"Invalid font, theme, Tab, recovery interval or recent-file settings."; return false; }
    for (auto zoom : p.zoom) if (zoom < 50 || zoom > 300) { if (error) *error = L"Zoom must be between 50% and 300%."; return false; }
    for (const auto& path : p.recent) if (path.empty() || path.size() > 32767 || path.find_first_of(L"\r\n") != std::wstring::npos || path.find(L'\0') != std::wstring::npos) { if (error) *error = L"Recent-file paths must be valid single-line values."; return false; }
    std::set<std::pair<unsigned,unsigned>> used;
    for (const auto& binding : p.shortcuts) if (binding.key) { Shortcut parsed; if (!ParseShortcut(FormatShortcut(binding),&parsed,error) || parsed.key != binding.key || parsed.modifiers != binding.modifiers || !used.insert({binding.key,binding.modifiers}).second || ((binding.modifiers == 1) && (binding.key == 'Z' || binding.key == 'Y' || binding.key == 'R' || binding.key == 'G' || binding.key == '0'))) { if (error) *error = L"Shortcut bindings must be unique and preserve Ctrl+Z/Y/R/G/0."; return false; } }
    return true;
}
bool LoadPreferences(const std::filesystem::path& path,AppPreferences* preferences,std::wstring* error) {
    std::vector<TextLine> lines; TextCodec codec; FileStamp stamp; if (!Read(path,&lines,&codec,&stamp,error)) return false; const auto fields = Extract(lines); AppPreferences result;
    const auto string = [&](const std::wstring& section,const std::wstring& key,std::wstring* value) { const auto found = fields.find(section + L"/" + Lower(key)); if (found != fields.end()) *value = found->second; };
    const auto number = [&](const std::wstring& key,unsigned fallback,unsigned maximum,unsigned* value) { const auto found = fields.find(kPreferences + L"/" + Lower(key)); *value = fallback; return found == fields.end() || Number(found->second,maximum,value); };
    bool valid = true; unsigned value = 0; string(kPreferences,L"Font",&result.font);
    valid = number(L"FontTenths",100,320,&result.font_tenths) && number(L"Theme",0,2,&result.theme) && number(L"TabWidth",4,16,&result.tab_width) && number(L"SnapshotSeconds",30,600,&result.snapshot_seconds);
    for (auto item : {std::make_pair(L"Wrap",&result.wrap),std::make_pair(L"Whitespace",&result.whitespace),std::make_pair(L"UseTabs",&result.use_tabs),std::make_pair(L"AutoRestore",&result.auto_restore),std::make_pair(L"RecoverDrafts",&result.recover_drafts)}) { valid = number(item.first,0,1,&value) && valid; *item.second = value != 0; }
    for (int i = 0; i < 3; ++i) { valid = number(L"Zoom" + std::to_wstring(i),100,300,&value) && valid; result.zoom[i] = static_cast<int>(value); }
    const auto count = fields.find(kRecent + L"/count"); unsigned recent = 0; if (count != fields.end() && !Number(count->second,20,&recent)) valid = false;
    for (unsigned i = 0; i < recent; ++i) { std::wstring path_value; string(kRecent,std::to_wstring(i),&path_value); result.recent.push_back(std::move(path_value)); }
    for (std::size_t i = 0; i < result.shortcuts.size(); ++i) { auto text = FormatShortcut(result.shortcuts[i]); string(kShortcuts,kActions[i],&text); if (!ParseShortcut(text,&result.shortcuts[i],error)) valid = false; }
    if (!valid || !ValidatePreferences(result,error)) { if (error && error->empty()) *error = L"Invalid portable preferences; defaults retained."; return false; }
    *preferences = std::move(result); return true;
}
bool SavePreferences(const std::filesystem::path& path,const AppPreferences& p,std::wstring* error) {
    if (!ValidatePreferences(p,error)) return false; std::vector<TextLine> lines; TextCodec codec; FileStamp stamp; if (!Read(path,&lines,&codec,&stamp,error)) return false;
    Fields updates; const auto field = [&](const std::wstring& section,const std::wstring& key,const std::wstring& value) { updates[section + L"/" + Lower(key)] = value; };
    field(kPreferences,L"Font",p.font); field(kPreferences,L"FontTenths",std::to_wstring(p.font_tenths)); field(kPreferences,L"Theme",std::to_wstring(p.theme)); field(kPreferences,L"TabWidth",std::to_wstring(p.tab_width)); field(kPreferences,L"SnapshotSeconds",std::to_wstring(p.snapshot_seconds));
    for (auto item : {std::make_pair(L"Wrap",p.wrap),std::make_pair(L"Whitespace",p.whitespace),std::make_pair(L"UseTabs",p.use_tabs),std::make_pair(L"AutoRestore",p.auto_restore),std::make_pair(L"RecoverDrafts",p.recover_drafts)}) field(kPreferences,item.first,std::to_wstring(item.second));
    for (int i = 0; i < 3; ++i) field(kPreferences,L"Zoom" + std::to_wstring(i),std::to_wstring(p.zoom[i]));
    field(kRecent,L"Count",std::to_wstring(p.recent.size())); for (unsigned i = 0; i < 20; ++i) field(kRecent,std::to_wstring(i),i < p.recent.size() ? p.recent[i] : L"");
    for (std::size_t i = 0; i < p.shortcuts.size(); ++i) field(kShortcuts,kActions[i],FormatShortcut(p.shortcuts[i]));
    std::set<std::wstring> applied; std::wstring section;
    for (auto& line : lines) { const auto text = Trim(line.text); if (text.empty() || text[0] == L';' || text[0] == L'#') continue; if (text.front() == L'[' && text.back() == L']') section = Lower(Trim(text.substr(1,text.size() - 2))); else { const auto equal = text.find(L'='); if (equal == std::wstring::npos) continue; const auto key = section + L"/" + Lower(Trim(text.substr(0,equal))); const auto found = updates.find(key); if (found != updates.end()) { const auto original_equal = line.text.find(L'='); line.text = line.text.substr(0,original_equal + 1) + found->second; applied.insert(key); } } }
    std::wstring text; for (const auto& line : lines) { text += line.text; text += line.ending; } if (!text.empty() && text.back() != L'\r' && text.back() != L'\n') text += L"\r\n";
    section.clear(); for (const auto& item : updates) if (!applied.count(item.first)) { const auto slash = item.first.find(L'/'); const auto next = item.first.substr(0,slash); if (section != next) { section = next; text += L"\r\n[" + section + L"]\r\n"; } text += item.first.substr(slash + 1) + L"=" + item.second + L"\r\n"; }
    std::vector<std::uint8_t> bytes; if (text.size() > 500000) { if (error) *error = L"Portable preferences exceed 1 MiB."; return false; }
    if (!EncodeText(text,codec,true,&bytes,error)) { codec = {}; codec.encoding = TextEncoding::Utf16Le; codec.bom = true; if (!EncodeText(text,codec,true,&bytes,error)) return false; if (error) error->clear(); }
    return WriteBytesSafely(path,bytes,error,{},&stamp);
}
}
