#pragma once
#include "binary_document.h"
namespace simple_editor::core {
struct IpsPreview { std::size_t offset = 0,length = 0; std::vector<std::uint8_t> before,after; };
struct IpsSummary { std::size_t records = 0,bytes = 0; bool ips32 = false; std::wstring base_sha256,result_sha256; std::vector<IpsPreview> preview; };
bool BuildIpsPatch(const std::vector<std::uint8_t>& base,const std::vector<std::uint8_t>& updated,bool ips32,const std::atomic_bool& cancel,std::vector<std::uint8_t>*,IpsSummary*,std::wstring*);
bool ApplyIpsPatch(BinaryDocument&,const std::vector<std::uint8_t>& patch,const std::wstring& expected_base_sha256,const SaveControl&,IpsSummary*,std::wstring*);
}
