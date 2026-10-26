#pragma once
#include "binary_document.h"
#include <set>
namespace simple_editor::core {
struct UnicodeNotice { std::size_t offset = 0,length = 0,line = 1,column = 1; std::uint32_t point = 0; std::wstring name; };
struct UnicodeNotices { std::vector<UnicodeNotice> rows; std::size_t total = 0; };
const wchar_t* UnicodeNoticeName(std::uint32_t);
std::vector<std::uint32_t> UnicodeNoticePoints();
bool ParseUnicodeAllowlist(const std::wstring&,std::set<std::uint32_t>*,std::wstring*);
bool ScanUnicodeNotices(const std::string&,const std::set<std::uint32_t>&,const SaveControl&,UnicodeNotices*,std::wstring*);
}
