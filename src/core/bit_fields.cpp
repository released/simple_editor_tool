#include "bit_fields.h"
#include "search.h"
#include <sstream>
#include <set>
namespace simple_editor::core {
namespace {
bool Valid(const BitField& field) { return (field.storage == 8 || field.storage == 16 || field.storage == 32 || field.storage == 64) && field.count && field.first < field.storage && field.count <= field.storage - field.first; }
std::uint64_t Mask(unsigned count) { return count == 64 ? UINT64_MAX : (1ULL << count) - 1; }
bool ReadStorage(const BinaryDocument& document,const BitField& field,std::vector<std::uint8_t>* bytes,std::uint64_t* value,std::wstring* error) {
    if (!Valid(field) || field.offset > document.Size() || field.storage / 8 > document.Size() - field.offset || !document.ValidateSource(true,error) || !document.Read(field.offset,field.storage / 8,bytes)) { if (error && error->empty()) *error = L"Invalid bitfield or storage outside the current document."; return false; }
    *value = 0; for (unsigned i = 0; i < field.storage / 8; ++i) *value |= static_cast<std::uint64_t>((*bytes)[i]) << (8 * (field.little ? i : field.storage / 8 - i - 1)); return true;
}
}
bool ParseBitFields(const std::wstring& definition,std::vector<BitField>* fields,std::wstring* error) {
    if (definition.size() > 32768) { if (error) *error = L"Bitfield definitions are limited to 32,768 characters."; return false; }
    std::wistringstream lines(definition); std::wstring line; std::vector<BitField> result; std::set<std::wstring> names; unsigned number = 0;
    while (std::getline(lines,line)) {
        ++number; if (line.find_first_not_of(L" \t\r") == std::wstring::npos || line[line.find_first_not_of(L" \t\r")] == L'#') continue;
        std::wistringstream input(line); BitField field; std::wstring offset,endian,extra;
        if (!(input >> field.name >> offset >> field.storage >> endian >> field.first >> field.count) || (input >> extra) || field.name.size() > 64 || !ParseAddress(offset,&field.offset) || (endian != L"le" && endian != L"be") || !Valid(field) || !names.insert(field.name).second || result.size() == 256) { if (error) *error = L"Invalid/duplicate field at line " + std::to_wstring(number) + L". Use: name byte-offset storage-bits le|be first-bit bit-count. Maximum 256 fields."; return false; }
        field.little = endian == L"le"; result.push_back(std::move(field));
    }
    if (result.empty()) { if (error) *error = L"Enter at least one named field."; return false; } *fields = std::move(result); return true;
}
bool ReadBitField(const BinaryDocument& document,const BitField& field,std::uint64_t* value,std::wstring* error) { std::vector<std::uint8_t> bytes; std::uint64_t storage; if (!ReadStorage(document,field,&bytes,&storage,error)) return false; *value = (storage >> field.first) & Mask(field.count); return true; }
bool WriteBitField(BinaryDocument& document,const BitField& field,std::uint64_t value,std::wstring* error) {
    std::vector<std::uint8_t> bytes; std::uint64_t storage;
    if (!ReadStorage(document,field,&bytes,&storage,error)) return false;
    if (value > Mask(field.count)) { if (error) *error = L"The value does not fit the selected unsigned field."; return false; }
    const auto updated = (storage & ~(Mask(field.count) << field.first)) | (value << field.first); if (updated == storage) return true;
    for (unsigned i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<std::uint8_t>(updated >> (8 * (field.little ? i : field.storage / 8 - i - 1)));
    if (!document.Replace(field.offset,bytes.size(),bytes)) { if (error) *error = L"Could not edit the bitfield; no bytes changed."; return false; } return true;
}
}
