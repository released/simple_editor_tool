#include "decimal_compare.h"
#include <algorithm>
namespace simple_editor::core {
namespace {
struct Decimal { std::wstring digits; int exponent = 0; bool negative = false; };
bool Parse(const std::wstring& text,Decimal* number) {
    if (text.empty() || text.size() > 256) return false; std::size_t at = 0; Decimal n; if (text[at] == L'-' || text[at] == L'+') n.negative = text[at++] == L'-'; bool digit = false,dot = false;
    while (at < text.size() && (text[at] == L'.' || (text[at] >= L'0' && text[at] <= L'9'))) { const auto c = text[at++]; if (c == L'.') { if (dot) return false; dot = true; } else { n.digits += c; digit = true; if (dot) --n.exponent; } }
    if (!digit) return false; if (at < text.size() && (text[at] == L'e' || text[at] == L'E')) { ++at; bool minus = false; if (at < text.size() && (text[at] == L'-' || text[at] == L'+')) minus = text[at++] == L'-'; int exponent = 0; const auto start = at; while (at < text.size() && text[at] >= L'0' && text[at] <= L'9') { exponent = exponent * 10 + text[at++] - L'0'; if (exponent > 4096) return false; } if (at == start) return false; n.exponent += minus ? -exponent : exponent; }
    if (at != text.size() || n.exponent < -4096 || n.exponent > 4096) return false; const auto first = n.digits.find_first_not_of(L'0'); if (first == std::wstring::npos) { n.digits = L"0"; n.negative = false; n.exponent = 0; } else n.digits.erase(0,first); *number = std::move(n); return true;
}
std::wstring Strip(std::wstring text) { const auto at = text.find_first_not_of(L'0'); return at == std::wstring::npos ? L"0" : text.substr(at); }
int Compare(const std::wstring& a,const std::wstring& b) { return a.size() != b.size() ? a.size() < b.size() ? -1 : 1 : a.compare(b); }
std::wstring Sum(std::wstring a,std::wstring b,bool subtract) { if (Compare(a,b) < 0) a.swap(b); b.insert(0,a.size() - b.size(),L'0'); int carry = 0; for (std::size_t i = a.size(); i--;) { int n = a[i] - L'0' + (subtract ? -(b[i] - L'0') : b[i] - L'0') + carry; if (subtract) { carry = n < 0 ? -1 : 0; if (n < 0) n += 10; } else { carry = n / 10; n %= 10; } a[i] = static_cast<wchar_t>(L'0' + n); } if (carry > 0) a.insert(a.begin(),L'1'); return Strip(std::move(a)); }
}
bool DecimalWithin(const std::wstring& left,const std::wstring& right,const std::wstring& tolerance,bool* equal) {
    Decimal a,b,t; if (!equal || !Parse(left,&a) || !Parse(right,&b) || !Parse(tolerance,&t) || t.negative) return false; const auto exponent = (std::min)({a.exponent,b.exponent,t.exponent}); for (auto* n : {&a,&b,&t}) { const auto zeros = n->exponent - exponent; if (n->digits.size() + zeros > 8448) return false; n->digits.append(zeros,L'0'); n->digits = Strip(std::move(n->digits)); }
    *equal = Compare(Sum(a.digits,b.digits,a.negative == b.negative),t.digits) <= 0; return true;
}
}
