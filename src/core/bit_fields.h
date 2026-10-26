#pragma once
#include "binary_document.h"
namespace simple_editor::core {
struct BitField {
    std::wstring name;
    std::uint64_t offset = 0;
    unsigned storage = 8, first = 0, count = 1;
    bool little = true;
};
bool ParseBitFields(const std::wstring&,std::vector<BitField>*,std::wstring*);
bool ReadBitField(const BinaryDocument&,const BitField&,std::uint64_t*,std::wstring*);
bool WriteBitField(BinaryDocument&,const BitField&,std::uint64_t,std::wstring*);
}
