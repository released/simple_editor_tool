#include "leb128.h"
#include <cstring>
namespace simple_editor::core {
bool DecodeLeb128(const std::uint8_t* bytes,std::size_t size,bool is_signed,Leb128Value* output) {
    if (!output || (!bytes && size)) return false;
    std::uint64_t value = 0;
    for (unsigned i = 0; i < 10 && i < size; ++i) {
        const unsigned payload = bytes[i] & 127;
        if (i == 9 && ((bytes[i] & 128) || (is_signed ? payload != 0 && payload != 127 : payload > 1))) return false;
        value |= static_cast<std::uint64_t>(i == 9 ? payload & 1 : payload) << (7 * i);
        if (!(bytes[i] & 128)) { if (is_signed && i < 9 && (payload & 64)) value |= UINT64_MAX << (7 * (i + 1)); Leb128Value result; result.length = i + 1; result.unsigned_value = value; std::memcpy(&result.signed_value,&value,sizeof(value)); *output = result; return true; }
    }
    return false;
}
}
