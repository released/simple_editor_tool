#pragma once
#include "binary_document.h"
namespace simple_editor::core {
struct CrcProfile { std::wstring name = L"CRC-32/ISO-HDLC"; unsigned width = 32; std::uint64_t polynomial = 0x04C11DB7,initial = 0xFFFFFFFF,xor_out = 0xFFFFFFFF; bool reflect_in = true,reflect_out = true; };
bool ParseCrcProfile(const std::wstring& text,CrcProfile* profile,std::wstring* error);
bool CalculateCrc(const BinaryDocument& document,std::uint64_t offset,std::uint64_t length,const CrcProfile& profile,const SaveControl& control,std::uint64_t* value,std::wstring* error);
}
