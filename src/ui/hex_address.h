#pragma once
#include <cstdint>
#include <cwchar>
#include <string>
namespace simple_editor::ui {
inline int HexAddressColumns(int digits) { return digits + digits / 4 - 1; }
inline std::wstring FormatHexAddress(std::uint64_t address,int digits) {
    const int width = digits > 8 || address > 0xFFFFFFFFULL ? 16 : 8;
    wchar_t buffer[17]{}; swprintf_s(buffer,L"%0*llX",width,static_cast<unsigned long long>(address));
    std::wstring result(buffer);
    for (std::size_t position = result.size() - 4; position; position -= 4) result.insert(position,1,L'_');
    return result;
}
}
