#pragma once
#include "file_operations.h"
namespace simple_editor::core {
std::wstring EncodeSetting(const std::wstring& value);
bool DecodeSetting(const std::wstring& encoded,std::wstring* value,std::size_t maximum = 65536);
struct NamedHexRange { std::uint64_t offset = 0,length = 0; unsigned color = 0x80D0F0; std::wstring name,comment; bool stale = false; };
struct HexBookmarks {
    FileStamp source; std::vector<NamedHexRange> ranges; bool stale = false;
    bool Add(const NamedHexRange& range,std::uint64_t size,std::wstring* error);
    std::wstring Save(bool disk_matches) const;
    bool Load(const std::wstring& value,std::uint64_t size,std::wstring* error);
    bool Matches(const BinaryDocument& document) const;
};
}
