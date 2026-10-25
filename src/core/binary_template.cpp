#include "binary_template.h"
#include "binary_analysis.h"
#include "line_merge.h"
#include "search.h"
#include <algorithm>
#include <cwctype>
#include <sstream>
namespace simple_editor::core {
namespace {
struct FieldSpec { std::wstring path,type,wanted; std::uint64_t offset = 0,stride = 0; unsigned count = 1,width = 1; bool little = true,bytes = false,text = false,ascii = false; };
bool Decimal(const std::wstring& text,unsigned maximum,unsigned* result) { if (text.empty() || text.find_first_not_of(L"0123456789") != std::wstring::npos) return false; try { const auto value = std::stoull(text); if (!value || value > maximum) return false; *result = static_cast<unsigned>(value); return true; } catch (...) { return false; } }
bool Type(FieldSpec* field) {
    auto type = field->type; for (auto& c : type) c = static_cast<wchar_t>(towlower(c)); field->type = type;
    const auto colon = type.find(L':'); if (colon != std::wstring::npos) { const auto word = type.substr(0,colon); field->bytes = word == L"bytes"; field->text = word == L"utf8" || word == L"ascii"; field->ascii = word == L"ascii"; return (field->bytes || field->text) && Decimal(type.substr(colon + 1),4096,&field->width); }
    if (type.size() > 2 && (type.substr(type.size() - 2) == L"le" || type.substr(type.size() - 2) == L"be")) { field->little = type.substr(type.size() - 2) == L"le"; type.resize(type.size() - 2); }
    if (type.size() < 2) return false; unsigned bits = 0; if (!Decimal(type.substr(1),64,&bits) || (bits != 8 && bits != 16 && bits != 32 && bits != 64) || (type[0] != L'u' && type[0] != L'i' && type[0] != L'f') || (type[0] == L'f' && bits < 32)) return false;
    field->width = bits / 8; field->wanted = (type[0] == L'u' ? L"uint" : type[0] == L'i' ? L"int" : L"float") + std::to_wstring(bits); return true;
}
bool Path(const std::wstring& path) { if (path.empty() || path.size() > 128 || path.front() == L'.' || path.back() == L'.' || path.find(L"..") != std::wstring::npos || std::count(path.begin(),path.end(),L'.') > 7) return false; const auto array = path.find(L"[]"); return array == std::wstring::npos || path.find(L"[]",array + 2) == std::wstring::npos; }
}
bool InspectBinaryTemplate(const BinaryDocument& document,const std::wstring& definition,std::uint64_t base,const SaveControl& control,BinaryTemplateResult* output,std::wstring* error) {
    if (error) error->clear(); if (definition.size() > 512 * 1024 || !ValidateText(definition,error) || !document.ValidateSource(true,error)) return false;
    std::vector<FieldSpec> specs; std::size_t total = 0,line_number = 0;
    for (const auto& line : SplitLines(definition)) {
        ++line_number; if (control.cancelled && *control.cancelled) { if (error) *error = L"Template inspection cancelled."; return false; }
        std::wistringstream input(line.text.substr(0,line.text.find(L'#'))); FieldSpec spec; std::wstring offset,count,stride,extra; if (!(input >> spec.path)) continue;
        const auto fail = [&]() { if (error) *error = L"Invalid binary template at line " + std::to_wstring(line_number) + L". Use path type offset [decimal-count [stride]]."; return false; };
        if (!(input >> spec.type >> offset) || !Path(spec.path) || !Type(&spec) || !ParseAddress(offset,&spec.offset)) return fail();
        if (input >> count) { if (!Decimal(count,4096,&spec.count)) return fail(); if (input >> stride) { if (!ParseAddress(stride,&spec.stride)) return fail(); if (input >> extra) return fail(); } else spec.stride = spec.width; } else spec.stride = spec.width;
        if (spec.offset > UINT64_MAX - base || (spec.count > 1 && spec.stride > (UINT64_MAX - base - spec.offset) / (spec.count - 1)) || spec.width > UINT64_MAX - base - spec.offset - static_cast<std::uint64_t>(spec.count - 1) * spec.stride || (total += spec.count) > 4096 || specs.size() >= 1024) return fail();
        specs.push_back(std::move(spec));
    }
    if (specs.empty()) { if (error) *error = L"The template contains no fields."; return false; }
    BinaryTemplateResult result; result.base = base;
    for (const auto& spec : specs) for (unsigned i = 0; i < spec.count; ++i) {
        if (control.cancelled && *control.cancelled) { if (error) *error = L"Template inspection cancelled."; return false; }
        BinaryField field; field.path = spec.path; const auto slot = field.path.find(L"[]"); if (slot != std::wstring::npos) field.path.replace(slot,2,L"[" + std::to_wstring(i) + L"]"); else if (spec.count > 1) field.path += L"[" + std::to_wstring(i) + L"]"; field.type = spec.type; field.offset = base + spec.offset + static_cast<std::uint64_t>(i) * spec.stride; field.length = spec.width;
        if (field.offset > document.Size() || field.length > document.Size() - field.offset) field.value = L"Not enough bytes";
        else if (spec.bytes || spec.text) {
            std::vector<std::uint8_t> data; if (!document.Read(field.offset,spec.width,&data) || data.size() != spec.width) { if (error) *error = L"Source changed while reading a template field."; return false; }
            if (spec.bytes) { for (std::size_t n = 0; n < (std::min<std::size_t>)(64,data.size()); ++n) { wchar_t value[4]{}; swprintf_s(value,L"%02X ",data[n]); field.value += value; } if (data.size() > 64) field.value += L"..."; field.valid = true; }
            else { data.resize(static_cast<std::size_t>(std::find(data.begin(),data.end(),0) - data.begin())); if (spec.ascii && std::any_of(data.begin(),data.end(),[](auto value) { return value >= 128; })) field.value = L"Invalid ASCII"; else { TextCodec codec; std::wstring detail; field.valid = DecodeText(data,TextEncoding::Utf8,&field.value,&codec,&detail); if (!field.valid) field.value = L"Invalid UTF-8"; } }
        } else { std::vector<InspectorValue> values; if (!InspectNumbers(document,field.offset,spec.little,&values,error)) return false; const auto found = std::find_if(values.begin(),values.end(),[&](const auto& value) { return value.type == spec.wanted; }); if (found != values.end()) { field.value = found->value; field.valid = found->valid; } }
        result.fields.push_back(std::move(field)); if (control.progress) control.progress(static_cast<unsigned>(result.fields.size() * 100 / total));
    }
    if (!document.ValidateSource(true,error)) return false; *output = std::move(result); return true;
}
}
