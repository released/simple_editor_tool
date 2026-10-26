#pragma once
#include "binary_document.h"
#include "text_codec.h"
namespace simple_editor::core {
struct TextPage { std::wstring text; std::uint64_t begin = 0,end = 0,total = 0; TextCodec codec; };
// Nonzero legacy-code-page offsets must come from a previously returned page end.
bool ReadTextPage(const BinaryDocument& source,std::uint64_t offset,TextEncoding encoding,bool known_boundary,const SaveControl& control,TextPage* result,std::wstring* error,std::size_t budget = 256 * 1024);
bool FindPagedText(const BinaryDocument&,const TextCodec&,const std::wstring& query,std::uint64_t start,const SaveControl&,std::uint64_t* found,std::uint64_t* after,std::wstring* error);
}
