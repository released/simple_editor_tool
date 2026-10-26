#include "language_catalog.h"
#include "line_merge.h"
#include "text_codec.h"
#include <algorithm>
#include <set>
namespace simple_editor::core {
std::wstring LanguageKey(const std::wstring& source) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const auto unit : source) { hash ^= static_cast<std::uint16_t>(unit); hash *= 1099511628211ULL; }
    wchar_t text[20]{}; swprintf_s(text,L"S%016llX",hash); return text;
}
namespace {
bool Unescape(const std::wstring& text,std::wstring* output) {
    std::wstring result;
    for (std::size_t i = 0; i < text.size(); ++i) { auto c = text[i]; if (c == L'\\') { if (++i == text.size()) return false; c = text[i]; if (c == L'n') c = L'\n'; else if (c == L'r') c = L'\r'; else if (c == L't') c = L'\t'; else if (c != L'\\') return false; } if (!c) return false; result += c; }
    if (result.size() > 16384) return false; *output = std::move(result); return true;
}
std::wstring Trim(std::wstring text) { const auto start = text.find_first_not_of(L" \t"); return start == std::wstring::npos ? L"" : text.substr(start,text.find_last_not_of(L" \t") - start + 1); }
bool PatternParts(const std::wstring& value,std::vector<std::wstring>* literals,std::vector<unsigned>* slots) {
    std::wstring literal; std::set<unsigned> used;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == L'{' && i + 2 < value.size() && value[i + 1] >= L'0' && value[i + 1] <= L'7' && value[i + 2] == L'}') { const unsigned slot = value[i + 1] - L'0'; if (!used.insert(slot).second || (!slots->empty() && literal.empty())) return false; literals->push_back(std::move(literal)); literal.clear(); slots->push_back(slot); i += 2; }
        else literal += value[i];
    } literals->push_back(std::move(literal)); return !slots->empty() && slots->size() <= 8;
}
bool PrintfPattern(const std::wstring& value,std::wstring* pattern,std::vector<std::wstring>* formats) {
    std::wstring result;
    for (std::size_t i = 0; i < value.size(); ++i) { if (value[i] != L'%') { result += value[i]; continue; } const auto start = i; if (++i == value.size()) return false; if (value[i] == L'%') { result += L'%'; continue; }
        while (i < value.size() && wcschr(L"-+ #0.123456789*hlLI64ztj",value[i])) ++i;
        if (i == value.size() || !wcschr(L"diouxXfFeEgGaAcCsSp",value[i]) || formats->size() == 8) return false;
        formats->push_back(value.substr(start,i - start + 1)); result += L"{" + std::to_wstring(formats->size() - 1) + L"}";
    } *pattern = std::move(result); return !formats->empty();
}
}
bool ParseLanguageFile(const std::wstring& text,LanguageFile* output,std::wstring* error) {
    const auto fail = [&]() { if (error) *error = L"Invalid language INI: metadata/version, duplicate key, escape, Unicode or size limit."; return false; };
    if (!output || text.size() > 500000 || !ValidateText(text,error)) return fail(); LanguageFile result; std::wstring section; bool version = false; std::set<std::wstring> metadata;
    for (const auto& line : SplitLines(text)) { const auto value = Trim(line.text); if (value.empty() || value[0] == L';' || value[0] == L'#') continue; if (value.front() == L'[' && value.back() == L']') { section = value.substr(1,value.size() - 2); if (section != L"Language" && section != L"Strings") return fail(); continue; }
        const auto equal = line.text.find(L'='); if (equal == std::wstring::npos) return fail(); const auto key = Trim(line.text.substr(0,equal)); const auto raw = line.text.substr(equal + 1);
        if (section == L"Language") { if (!metadata.insert(key).second) return fail(); if (key == L"Id") result.id = Trim(raw); else if (key == L"Name") result.name = Trim(raw); else if (key == L"Version") version = Trim(raw) == L"1"; else return fail(); }
        else if (section == L"Strings") { std::wstring decoded; if (key.size() != 17 || key[0] != L'S' || key.substr(1).find_first_not_of(L"0123456789ABCDEF") != std::wstring::npos || !Unescape(raw,&decoded) || decoded.empty() || result.strings.size() >= 4096 || !result.strings.emplace(key,std::move(decoded)).second) return fail(); }
        else return fail();
    }
    if (!version || (result.id != L"en" && result.id != L"tw") || result.name.empty() || result.name.size() > 80 || result.strings.empty()) return fail(); *output = std::move(result); return true;
}
bool ReadLanguageFile(const std::filesystem::path& path,LanguageFile* output,std::wstring* error) {
    BinaryDocument document; std::vector<std::uint8_t> bytes; std::wstring text; TextCodec codec;
    if (!document.Open(path.wstring(),error) || document.Size() > 1024 * 1024 || !document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) || !DecodeText(bytes,TextEncoding::Auto,&text,&codec,error) || !document.ValidateSource(true,error)) { if (error && error->empty()) *error = L"Language INI could not be read or exceeds 1 MiB."; return false; }
    return ParseLanguageFile(text,output,error);
}
bool LanguageCatalog::Load(const std::filesystem::path& directory,std::wstring* error) {
    LanguageFile en,tw; if (!ReadLanguageFile(directory / L"lang_en.ini",&en,error) || en.id != L"en" || !ReadLanguageFile(directory / L"lang_tw.ini",&tw,error) || tw.id != L"tw") return false;
    for (const auto& entry : tw.strings) if (!en.strings.count(entry.first)) { if (error) *error = L"Traditional Chinese language INI contains a key absent from English."; return false; }
    english_ = std::move(en); traditional_ = std::move(tw); BuildPatterns(); return true;
}
bool LanguageCatalog::Select(const std::wstring& id) { if (id != L"en" && id != L"tw") return false; selected_ = id; BuildPatterns(); return true; }
void LanguageCatalog::BuildPatterns() {
    patterns_.clear();
    for (const auto& source : english_.strings) {
        if (LanguageKey(source.second) != source.first) continue;
        const auto translated = traditional_.strings.find(source.first); const auto& target = selected_ == L"tw" && translated != traditional_.strings.end() ? translated->second : source.second;
        std::wstring source_pattern = source.second,target_pattern = target; std::vector<std::wstring> source_formats,target_formats;
        const bool printf_source = PrintfPattern(source.second,&source_pattern,&source_formats),printf_target = PrintfPattern(target,&target_pattern,&target_formats);
        if (printf_source && (!printf_target || source_formats != target_formats)) continue;
        Pattern pattern; std::vector<std::wstring> target_literals; std::vector<unsigned> target_slots;
        if (!PatternParts(source_pattern,&pattern.literals,&pattern.slots) || !PatternParts(target_pattern,&target_literals,&target_slots)) continue;
        auto a = pattern.slots,b = target_slots; std::sort(a.begin(),a.end()); std::sort(b.begin(),b.end()); if (a != b || (pattern.literals.front().size() < 6 && (!pattern.literals.front().empty() || pattern.literals.size() < 2 || pattern.literals[1].size() < 8))) continue;
        pattern.target = std::move(target_pattern); patterns_.push_back(std::move(pattern));
    }
    std::sort(patterns_.begin(),patterns_.end(),[](const auto& a,const auto& b) { return a.literals.front().size() > b.literals.front().size(); });
}
std::wstring LanguageCatalog::Translate(const std::wstring& source) const {
    if (source.empty() || source.size() > 32768) return source;
    const auto key = LanguageKey(source); const auto& selected = selected_ == L"tw" ? traditional_ : english_; const auto exact = selected.strings.find(key); if (exact != selected.strings.end()) return exact->second;
    const auto fallback = english_.strings.find(key); if (fallback != english_.strings.end()) return fallback->second;
    for (const auto& pattern : patterns_) {
        if (source.compare(0,pattern.literals[0].size(),pattern.literals[0]) != 0) continue; std::size_t at = pattern.literals[0].size(); std::wstring values[8]; bool matched = true;
        for (std::size_t i = 0; i < pattern.slots.size(); ++i) { const auto& next = pattern.literals[i + 1]; const auto end = next.empty() ? source.size() : source.find(next,at); if (end == std::wstring::npos || (i + 1 == pattern.slots.size() && end + next.size() != source.size())) { matched = false; break; } values[pattern.slots[i]] = source.substr(at,end - at); at = end + next.size(); }
        if (!matched || at != source.size()) continue; std::wstring result;
        for (std::size_t i = 0; i < pattern.target.size(); ++i) { if (pattern.target[i] == L'{' && i + 2 < pattern.target.size() && pattern.target[i + 1] >= L'0' && pattern.target[i + 1] <= L'7' && pattern.target[i + 2] == L'}') { result += values[pattern.target[i + 1] - L'0']; i += 2; } else result += pattern.target[i]; } return result;
    }
    return source;
}
}
