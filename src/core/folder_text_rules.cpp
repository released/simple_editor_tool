#include "folder_text_rules.h"
#include <algorithm>
namespace simple_editor::core {
bool ParseFolderTextSettings(const std::wstring& text,FolderTextSettings* output,std::wstring* error) {
    if (text.size() > 65536 || !ValidateText(text,error)) return false; FolderTextSettings result; std::wstring literals;
    for (const auto& line : SplitLines(text)) { if (line.text.empty() || line.text[0] == L'#') continue; const auto equal = line.text.find(L'='); if (equal == std::wstring::npos) { if (error) *error = L"Use setting=value on each line."; return false; } const auto key = line.text.substr(0,equal),value = line.text.substr(equal + 1);
        if (key == L"include") { if (value.empty() || value.size() > 4096) return false; result.include = value; }
        else if (key == L"enabled" || key == L"ignore-space" || key == L"ignore-case" || key == L"ignore-eol") { if (value != L"yes" && value != L"no") return false; const bool flag = value == L"yes"; if (key == L"enabled") result.enabled = flag; else if (key == L"ignore-space") result.options.ignore_space = flag; else if (key == L"ignore-case") result.options.ignore_case = flag; else result.options.ignore_eol = flag; }
        else if (key == L"encoding") { const std::wstring names[] = {L"auto",L"utf8",L"utf16le",L"utf16be",L"big5",L"system"}; const auto found = std::find(std::begin(names),std::end(names),value); if (found == std::end(names)) return false; result.encoding = static_cast<TextEncoding>(found - std::begin(names)); }
        else literals += line.text + L"\n";
    } if (!ParseTextRules(literals,&result.options.rules,error)) return false; *output = std::move(result); return true;
}
bool EvaluateFolderText(const std::filesystem::path& left,const std::filesystem::path& right,const FolderTextSettings& settings,bool raw_equal,const std::atomic_bool& cancel,FolderRow* row,std::wstring* error) {
    if (!settings.enabled || !MatchFileFilter(row->relative.filename().wstring(),settings.include)) return true;
    if (raw_equal) { row->text_state = L"Byte-identical"; return true; }
    const auto decode = [&](const std::filesystem::path& path,std::wstring* text) { BinaryDocument document; TextCodec codec; std::vector<std::uint8_t> bytes; if (cancel) return false; if (!document.Open(path.wstring(),&row->text_error)) return false; if (document.Size() > 16 * 1024 * 1024) { row->text_error = L"Text-rule evaluation is limited to 16 MiB per input."; return false; } return document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) && DecodeText(bytes,settings.encoding,text,&codec,&row->text_error) && document.ValidateSource(true,&row->text_error); };
    std::wstring a,b; TextMergeModel model; if (!decode(left,&a) || !decode(right,&b) || !BuildTextMerge(L"",a,b,false,settings.options,cancel,&model,&row->text_error)) { if (cancel) { if (error) *error = L"Folder text analysis cancelled."; return false; } row->text_state = L"Text analysis error"; if (row->text_error.empty()) row->text_error = L"Could not decode/read the selected text pair."; return true; }
    bool identical = a == b; if (identical) { BinaryDocument first,second; std::vector<std::uint8_t> x,y; if (!first.Open(left.wstring(),&row->text_error) || !second.Open(right.wstring(),&row->text_error) || first.Size() > 16 * 1024 * 1024 || second.Size() > 16 * 1024 * 1024 || !first.Read(0,static_cast<std::size_t>(first.Size()),&x) || !second.Read(0,static_cast<std::size_t>(second.Size()),&y)) { row->text_state = L"Text analysis error"; return true; } identical = x == y; }
    row->text_state = identical ? L"Byte-identical" : std::none_of(model.blocks.begin(),model.blocks.end(),[](const auto& block) { return block.changed && block.important; }) ? L"Equal under text rules" : L"Important text differences"; return true;
}
}
