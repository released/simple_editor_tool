#pragma once
#include "text_codec.h"
#include "binary_document.h"
namespace simple_editor::core {
struct TextByteRange { std::uint64_t text_begin = 0,text_end = 0,byte_begin = 0,byte_end = 0; };
bool MapTextByteRange(const std::wstring& raw,const TextCodec& codec,bool from_text,std::uint64_t begin,std::uint64_t end,const SaveControl&,TextByteRange*,std::wstring*);
}
