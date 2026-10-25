#pragma once
#include "binary_document.h"
#include "text_codec.h"
namespace simple_editor::core {
enum class HexDecoding { None, Utf8, Utf16Le, Utf16Be, Big5 };
struct DecodedGlyph { std::uint64_t offset = 0; unsigned length = 1; std::wstring text; bool valid = false; };
bool DecodeHexWindow(const BinaryDocument&,std::uint64_t,std::size_t,HexDecoding,std::vector<DecodedGlyph>*,std::wstring*);
enum class IntegerOperation { Xor, And, Or, Add, Subtract, ShiftLeft, ShiftRight };
struct IntegerTransform { IntegerOperation operation = IntegerOperation::Xor; unsigned width = 8; bool little = true; std::uint64_t operand = 0; };
bool TransformIntegers(BinaryDocument&,std::uint64_t,std::uint64_t,const IntegerTransform&,const SaveControl&,std::wstring*);
struct ExtractedString { std::uint64_t offset = 0,length = 0; std::wstring text,encoding; };
struct StringExtraction { std::vector<ExtractedString> rows; std::uint64_t total = 0; bool truncated = false; };
bool ExtractStrings(const BinaryDocument&,std::uint64_t,std::uint64_t,unsigned,unsigned,const SaveControl&,StringExtraction*,std::wstring*);
struct ModifiedRange { std::uint64_t before = 0,removed = 0,offset = 0,length = 0; std::wstring state; };
bool FindModifiedRanges(const BinaryDocument&,const BinaryDocument&,const SaveControl&,std::vector<ModifiedRange>*,std::wstring*);
}
