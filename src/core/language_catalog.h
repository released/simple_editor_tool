#pragma once
#include "file_operations.h"
#include <map>
namespace simple_editor::core {
struct LanguageFile { std::wstring id,name; std::map<std::wstring,std::wstring> strings; };
std::wstring LanguageKey(const std::wstring& source);
bool ParseLanguageFile(const std::wstring&,LanguageFile*,std::wstring* error);
bool ReadLanguageFile(const std::filesystem::path&,LanguageFile*,std::wstring* error);
class LanguageCatalog {
public:
    bool Load(const std::filesystem::path& directory,std::wstring* error);
    bool Select(const std::wstring& id);
    std::wstring Translate(const std::wstring& source) const;
    const std::wstring& Selected() const { return selected_; }
    std::size_t Entries() const { return english_.strings.size(); }
private:
    struct Pattern { std::vector<std::wstring> literals; std::vector<unsigned> slots; std::wstring target; };
    LanguageFile english_,traditional_;
    std::vector<Pattern> patterns_;
    std::wstring selected_ = L"en";
    void BuildPatterns();
};
}
