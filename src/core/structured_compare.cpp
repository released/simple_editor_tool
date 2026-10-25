#include "structured_compare.h"
#include "text_codec.h"
#include "table_compare.h"
#include <algorithm>
#include <map>
#include <set>
namespace simple_editor::core {
namespace {
constexpr std::size_t kNodes = 200000,kResults = 20000;
std::wstring Short(const std::wstring& text,std::size_t limit = 512) { if (text.size() <= limit) return text; if (limit && text[limit - 1] >= 0xD800 && text[limit - 1] <= 0xDBFF) --limit; return text.substr(0,limit) + L"..."; }
std::wstring Quote(const std::wstring& value,bool shorten = true) {
    const auto text = shorten ? Short(value) : value; std::wstring result = L"\"";
    for (auto c : text) { if (c == L'\"' || c == L'\\') { result += L'\\'; result += c; } else if (c < 32) { wchar_t escaped[7]{}; swprintf_s(escaped,L"\\u%04X",static_cast<unsigned>(c)); result += escaped; } else result += c; }
    return result + L"\"";
}
struct Failure { std::wstring text; };
struct Json {
    enum class Kind { Null,Bool,Number,String,Array,Object } kind = Kind::Null;
    std::wstring scalar;
    std::vector<Json> array;
    std::map<std::wstring,Json> object;
};
class JsonParser {
public:
    JsonParser(const std::wstring& text,const std::atomic_bool& cancel) : text_(text),cancel_(cancel) {}
    Json Parse() { auto result = Value(0); Space(); if (at_ != text_.size()) Fail(); return result; }
private:
    void Fail() { throw Failure{L"Invalid JSON near UTF-16 position " + std::to_wstring(at_) + L"."}; }
    void Check() { if (cancel_) throw Failure{L"Structured comparison cancelled."}; }
    void Space() { while (at_ < text_.size() && (text_[at_] == L' ' || text_[at_] == L'\t' || text_[at_] == L'\r' || text_[at_] == L'\n')) { ++at_; if (!(at_ & 1023)) Check(); } }
    bool Take(wchar_t c) { Space(); if (at_ < text_.size() && text_[at_] == c) { ++at_; return true; } return false; }
    bool Digit() const { return at_ < text_.size() && text_[at_] >= L'0' && text_[at_] <= L'9'; }
    std::wstring String() {
        if (!Take(L'\"')) Fail(); std::wstring value; bool closed = false;
        while (at_ < text_.size()) { if (!(at_ & 1023)) Check(); auto c = text_[at_++]; if (c == L'\"') { closed = true; break; } if (c < 32) Fail();
            if (c == L'\\') { if (at_ == text_.size()) Fail(); c = text_[at_++]; if (c == L'u') { unsigned number = 0; for (unsigned i = 0; i < 4; ++i) { if (at_ == text_.size()) Fail(); const auto d = text_[at_++]; const int digit = d >= L'0' && d <= L'9' ? d - L'0' : d >= L'a' && d <= L'f' ? d - L'a' + 10 : d >= L'A' && d <= L'F' ? d - L'A' + 10 : -1; if (digit < 0) Fail(); number = number * 16 + digit; } c = static_cast<wchar_t>(number); }
                else if (c == L'b') c = L'\b'; else if (c == L'f') c = L'\f'; else if (c == L'n') c = L'\n'; else if (c == L'r') c = L'\r'; else if (c == L't') c = L'\t'; else if (c != L'\"' && c != L'\\' && c != L'/') Fail(); }
            value += c;
        }
        if (!closed) Fail(); for (std::size_t i = 0; i < value.size(); ++i) if (value[i] >= 0xD800 && value[i] <= 0xDBFF) { if (++i == value.size() || value[i] < 0xDC00 || value[i] > 0xDFFF) Fail(); } else if (value[i] >= 0xDC00 && value[i] <= 0xDFFF) Fail(); return value;
    }
    std::wstring Number() {
        const bool negative = text_[at_] == L'-'; if (negative) ++at_; if (!Digit()) Fail(); std::wstring digits;
        if (text_[at_] == L'0') digits += text_[at_++]; else while (Digit()) { digits += text_[at_++]; if (!(at_ & 1023)) Check(); }
        std::int64_t fraction = 0,exponent = 0;
        if (at_ < text_.size() && text_[at_] == L'.') { ++at_; if (!Digit()) Fail(); while (Digit()) { digits += text_[at_++]; ++fraction; if (!(at_ & 1023)) Check(); } }
        if (at_ < text_.size() && (text_[at_] == L'e' || text_[at_] == L'E')) { ++at_; bool minus = false; if (at_ < text_.size() && (text_[at_] == L'+' || text_[at_] == L'-')) minus = text_[at_++] == L'-'; if (!Digit()) Fail(); while (Digit()) { exponent = exponent * 10 + text_[at_++] - L'0'; if (exponent > 1000000) throw Failure{L"JSON exponent exceeds the supported +/-1,000,000 range."}; if (!(at_ & 1023)) Check(); } if (minus) exponent = -exponent; }
        const auto nonzero = digits.find_first_not_of(L'0'); if (nonzero == std::wstring::npos) return L"0"; digits.erase(0,nonzero); exponent -= fraction; while (digits.size() > 1 && digits.back() == L'0') { digits.pop_back(); ++exponent; }
        return (negative ? L"-" : L"") + digits + (exponent ? L"e" + std::to_wstring(exponent) : L"");
    }
    Json Value(unsigned depth) {
        Check(); if (depth > 128 || ++nodes_ > kNodes) throw Failure{L"JSON is limited to 128 nesting levels and 200,000 nodes per input."}; Space(); if (at_ == text_.size()) Fail(); Json result; const auto c = text_[at_];
        if (c == L'\"') { result.kind = Json::Kind::String; result.scalar = Quote(String(),false); }
        else if (c == L'[') { result.kind = Json::Kind::Array; ++at_; if (!Take(L']')) { do { result.array.push_back(Value(depth + 1)); } while (Take(L',')); if (!Take(L']')) Fail(); } }
        else if (c == L'{') { result.kind = Json::Kind::Object; ++at_; if (!Take(L'}')) { do { const auto key = String(); if (!Take(L':')) Fail(); auto value = Value(depth + 1); if (!result.object.emplace(key,std::move(value)).second) throw Failure{L"Duplicate JSON object keys are ambiguous and are rejected."}; } while (Take(L',')); if (!Take(L'}')) Fail(); } }
        else if (c == L'-' || Digit()) { result.kind = Json::Kind::Number; result.scalar = Number(); }
        else { const std::wstring word = c == L'n' ? L"null" : c == L't' ? L"true" : c == L'f' ? L"false" : L""; if (word.empty() || text_.compare(at_,word.size(),word) != 0) Fail(); at_ += word.size(); result.kind = c == L'n' ? Json::Kind::Null : Json::Kind::Bool; result.scalar = word; }
        return result;
    }
    const std::wstring& text_; const std::atomic_bool& cancel_; std::size_t at_ = 0,nodes_ = 0;
};
using Table = std::vector<std::vector<std::wstring>>;
Table ParseTable(const std::wstring& text,wchar_t delimiter,const std::atomic_bool& cancel) {
    Table rows; std::size_t at = 0,cells = 0;
    while (at < text.size()) {
        std::vector<std::wstring> row; bool more = true;
        while (more) { if (cancel) throw Failure{L"Structured comparison cancelled."}; std::wstring value;
            if (at < text.size() && text[at] == L'\"') { ++at; bool closed = false; while (at < text.size()) { if (!(at & 1023) && cancel) throw Failure{L"Structured comparison cancelled."}; auto c = text[at++]; if (c == L'\"') { if (at < text.size() && text[at] == L'\"') ++at; else { closed = true; break; } } value += c; } if (!closed || (at < text.size() && text[at] != delimiter && text[at] != L'\r' && text[at] != L'\n')) throw Failure{L"Unterminated quoted field or unexpected characters after a closing quote."}; }
            else while (at < text.size() && text[at] != delimiter && text[at] != L'\r' && text[at] != L'\n') { if (!(at & 1023) && cancel) throw Failure{L"Structured comparison cancelled."}; if (text[at] == L'\"') throw Failure{L"Quotes inside an unquoted delimited field are invalid."}; value += text[at++]; }
            if (++cells > kNodes) throw Failure{L"Delimited input is limited to 200,000 cells per side."}; row.push_back(std::move(value)); more = at < text.size() && text[at] == delimiter; if (more) ++at;
        }
        rows.push_back(std::move(row)); if (at < text.size() && text[at] == L'\r') { ++at; if (at < text.size() && text[at] == L'\n') ++at; } else if (at < text.size() && text[at] == L'\n') ++at;
    }
    return rows;
}
void Add(StructuredComparison* result,const std::wstring& path,const std::wstring& left,const std::wstring& right,const wchar_t* state) { ++result->different; if (result->rows.size() < kResults) result->rows.push_back({Short(path,2048),Short(left),Short(right),state}); else result->truncated = true; }
std::wstring Display(const Json* value) { return !value ? L"<missing>" : value->kind == Json::Kind::Array ? L"[array]" : value->kind == Json::Kind::Object ? L"{object}" : Short(value->scalar); }
void CompareJson(const Json* left,const Json* right,const std::wstring& path,const std::atomic_bool& cancel,StructuredComparison* result) {
    if (cancel) throw Failure{L"Structured comparison cancelled."}; ++result->compared;
    if (!left || !right || left->kind != right->kind) { Add(result,path,Display(left),Display(right),!left ? L"Added" : !right ? L"Removed" : L"Type changed"); return; }
    if (left->kind == Json::Kind::Object) { std::set<std::wstring> keys; for (const auto& pair : left->object) keys.insert(pair.first); for (const auto& pair : right->object) keys.insert(pair.first); for (const auto& key : keys) { const auto a = left->object.find(key),b = right->object.find(key); CompareJson(a == left->object.end() ? nullptr : &a->second,b == right->object.end() ? nullptr : &b->second,Short(path + L"[" + Quote(key) + L"]",2048),cancel,result); } }
    else if (left->kind == Json::Kind::Array) for (std::size_t i = 0; i < (std::max)(left->array.size(),right->array.size()); ++i) CompareJson(i < left->array.size() ? &left->array[i] : nullptr,i < right->array.size() ? &right->array[i] : nullptr,path + L"[" + std::to_wstring(i) + L"]",cancel,result);
    else if (left->scalar != right->scalar) Add(result,path,Display(left),Display(right),L"Changed");
}
}
bool ReadDelimitedTable(const std::wstring& text,wchar_t delimiter,const std::atomic_bool& cancel,std::vector<std::vector<std::wstring>>* rows,std::wstring* error) { try { *rows = ParseTable(text,delimiter,cancel); return true; } catch (const Failure& failure) { if (error) *error = failure.text; return false; } }
bool CompareStructured(const std::wstring& left,const std::wstring& right,StructuredFormat format,const std::atomic_bool& cancel,StructuredComparison* output,std::wstring* error,const TableCompareOptions& options) {
    if (error) error->clear();
    if (left.size() > 16 * 1024 * 1024 || right.size() > 16 * 1024 * 1024 || static_cast<unsigned>(format) > 2 || !ValidateText(left,error) || !ValidateText(right,error)) { if (error && error->empty()) *error = L"Invalid or oversized structured input."; return false; }
    try { StructuredComparison result;
        if (format != StructuredFormat::Json && (!options.columns.empty() || options.header)) return CompareMappedTable(left,right,format,options,cancel,output,error);
        if (format == StructuredFormat::Json) { const auto a = JsonParser(left,cancel).Parse(),b = JsonParser(right,cancel).Parse(); CompareJson(&a,&b,L"$",cancel,&result); }
        else { const auto a = ParseTable(left,format == StructuredFormat::Csv ? L',' : L'\t',cancel),b = ParseTable(right,format == StructuredFormat::Csv ? L',' : L'\t',cancel); for (std::size_t row = 0; row < (std::max)(a.size(),b.size()); ++row) { if (cancel) throw Failure{L"Structured comparison cancelled."}; const auto* x = row < a.size() ? &a[row] : nullptr; const auto* y = row < b.size() ? &b[row] : nullptr; const auto columns = (std::max)(x ? x->size() : 0,y ? y->size() : 0); for (std::size_t col = 0; col < columns; ++col) { ++result.compared; const auto* first = x && col < x->size() ? &(*x)[col] : nullptr; const auto* second = y && col < y->size() ? &(*y)[col] : nullptr; if (!first || !second || *first != *second) Add(&result,L"Row " + std::to_wstring(row + 1) + L", column " + std::to_wstring(col + 1),first ? Quote(*first) : L"<missing>",second ? Quote(*second) : L"<missing>",!first ? L"Added" : !second ? L"Removed" : L"Changed"); } } }
        if (cancel) throw Failure{L"Structured comparison cancelled."}; *output = std::move(result); return true;
    } catch (const Failure& failure) { if (error) *error = failure.text; return false; } catch (const std::exception&) { if (error) *error = L"Structured comparison exceeded available memory."; return false; }
}
}
