#pragma once
#include <cstdint>
#include <cstddef>
namespace simple_editor::core {
struct Leb128Value { std::uint64_t unsigned_value = 0; std::int64_t signed_value = 0; unsigned length = 0; };
bool DecodeLeb128(const std::uint8_t* bytes,std::size_t size,bool is_signed,Leb128Value*);
}
