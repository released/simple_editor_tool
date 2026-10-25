#include "native_search.h"
#include "code_editor.h"
#include "../core/line_merge.h"
#include <algorithm>
#include <memory>
#include <array>
#include "../../vendor/scintilla/src/Position.h"
#include "../../vendor/scintilla/src/CharClassify.h"
#include "../../vendor/scintilla/src/RESearch.h"

bool DecodeSearchEscapes(const std::wstring& input,std::wstring* output,std::wstring* error) {
    std::wstring result;
    for (std::size_t i = 0; i < input.size(); ++i) { auto ch = input[i];
        if (ch == L'\\') { if (++i == input.size()) { if (error) *error = L"Trailing escape character."; return false; } ch = input[i];
            if (ch == L'n') ch = L'\n'; else if (ch == L'r') ch = L'\r'; else if (ch == L't') ch = L'\t';
            else if (ch == L'x' || ch == L'u') { const auto digits = ch == L'x' ? 2u : 4u; unsigned code = 0; for (unsigned j = 0; j < digits; ++j) { if (++i >= input.size()) { if (error) *error = L"Incomplete hexadecimal escape."; return false; } const auto c = towupper(input[i]); const int digit = c >= L'0' && c <= L'9' ? c - L'0' : c >= L'A' && c <= L'F' ? c - L'A' + 10 : -1; if (digit < 0) { if (error) *error = L"Invalid hexadecimal escape."; return false; } code = code * 16 + digit; } ch = static_cast<wchar_t>(code); }
            else if (ch != L'\\') { if (error) *error = L"Supported escapes: \\n \\r \\t \\xHH \\uHHHH \\\\."; return false; }
        }
        if (!ch) { if (error) *error = L"Text search cannot insert NUL. Use Hex mode."; return false; } result.push_back(ch);
    }
    if (!result.empty() && CCodeEditor::ToUtf8(result).empty()) { if (error) *error = L"Escapes contain an invalid Unicode surrogate."; return false; } *output = std::move(result); return true;
}
bool PlanEditorSearch(const std::string& input,const NativeSearchOptions& options,std::uint64_t begin,std::uint64_t end,std::uint64_t selected_start,std::uint64_t selected_end,const std::atomic_bool& cancel,const std::function<void(unsigned)>& progress,NativeSearchPlan* plan,std::wstring* error) {
    if (!CCodeEditor::InitializeResources()) { if (error) *error = L"Could not initialize the native text search engine."; return false; }
    if (input.size() > 128 * 1024 * 1024 || begin > end || end > input.size() || options.find.empty()) { if (error) *error = L"Invalid text search range or empty pattern."; return false; }
    std::wstring find = options.find, replacement = options.replacement;
    if (options.syntax == 1 && (!DecodeSearchEscapes(find,&find,error) || !DecodeSearchEscapes(replacement,&replacement,error))) return false;
    if (options.syntax != 2) find = simple_editor::core::TextForEditControl(find); replacement = simple_editor::core::TextForEditControl(replacement);
    const auto pattern = CCodeEditor::ToUtf8(find), replace = CCodeEditor::ToUtf8(replacement);
    if (pattern.empty() || (!replacement.empty() && replace.empty()) || pattern.size() > 32768) { if (error) *error = L"Invalid Unicode or search pattern exceeds 32 KiB."; return false; }
    if (options.syntax == 2) { Scintilla::Internal::CharClassify classes; Scintilla::Internal::RESearch regex(&classes); const char* reason = regex.Compile(pattern.data(),static_cast<Sci::Position>(pattern.size()),options.match_case,true); if (reason) { if (error) *error = L"Invalid regex: " + CCodeEditor::FromUtf8(reason); return false; } }
    struct SearchWindow { HWND window = nullptr; ~SearchWindow() { if (window) DestroyWindow(window); } } control;
    control.window = CreateWindowExW(0,L"Scintilla",L"",WS_POPUP,0,0,0,0,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if (!control.window) { if (error) *error = L"Could not create the isolated native search document."; return false; }
    const auto call = [&](UINT message,WPARAM a = 0,LPARAM b = 0) { return SendMessageW(control.window,message,a,b); };
    call(SCI_SETCODEPAGE,SC_CP_UTF8); call(SCI_SETCOMMANDEVENTS,0); call(SCI_SETUNDOCOLLECTION,0); call(SCI_SETTEXT,0,reinterpret_cast<LPARAM>(input.c_str()));
    call(SCI_SETSEARCHFLAGS,(options.match_case ? SCFIND_MATCHCASE : 0) | (options.whole_word ? SCFIND_WHOLEWORD : 0) | (options.syntax == 2 ? SCFIND_REGEXP | SCFIND_POSIX : 0));
    const auto search = [&](std::uint64_t a,std::uint64_t b) { call(SCI_SETTARGETRANGE,static_cast<WPARAM>(a),static_cast<LPARAM>(b)); return call(SCI_SEARCHINTARGET,pattern.size(),reinterpret_cast<LPARAM>(pattern.data())); };
    NativeSearchPlan result;
    if (options.action <= 1) {
        auto start = (std::clamp<std::uint64_t>)(options.previous ? selected_start : selected_end,begin,end); LRESULT at = -1;
        if (options.action == 1 && selected_start >= begin && selected_end <= end && search(selected_start,selected_end) == static_cast<LRESULT>(selected_start) && call(SCI_GETTARGETEND) == static_cast<LRESULT>(selected_end)) {
            if (cancel) { if (error) *error = L"Search cancelled."; return false; }
            call(options.syntax == 2 ? SCI_REPLACETARGETRE : SCI_REPLACETARGET,replace.size(),reinterpret_cast<LPARAM>(replace.data())); result.count = 1; result.cursor = call(SCI_GETTARGETEND); result.changed = true;
        } else {
            at = search(start,options.previous ? begin : end);
            if (at < 0 && options.wrap && !cancel && start != (options.previous ? end : begin)) { result.wrapped = true; at = search(options.previous ? end : begin,start); }
            if (at >= 0) { result.found = true; result.count = 1; result.cursor = at; result.length = static_cast<std::uint64_t>(call(SCI_GETTARGETEND) - at); }
        }
    } else {
        auto stop = end; auto position = begin;
        while (position <= stop && !cancel) {
            const auto at = search(position,stop); if (at < 0) break; const auto last = static_cast<std::uint64_t>(call(SCI_GETTARGETEND));
            if (at < static_cast<LRESULT>(position) || last < static_cast<std::uint64_t>(at) || last > stop) { if (error) *error = L"Native search returned an invalid range."; return false; }
            ++result.count;
            if (options.action == 4 && result.matches.size() < 100000) {
                NativeSearchMatch match; match.offset = at; match.length = last - at; match.line = call(SCI_LINEFROMPOSITION,at) + 1;
                const auto line_end = call(SCI_GETLINEENDPOSITION,match.line - 1); const auto preview_end = (std::min<LRESULT>)(line_end,at + 256); const auto safe_end = preview_end < line_end ? call(SCI_POSITIONBEFORE,preview_end) : preview_end;
                std::vector<char> text(static_cast<std::size_t>((std::max<LRESULT>)(0,safe_end - at)) + 1); Sci_TextRangeFull range{{at,safe_end},text.data()}; call(SCI_GETTEXTRANGEFULL,0,reinterpret_cast<LPARAM>(&range)); match.preview = text.data(); result.matches.push_back(std::move(match));
            }
            if (options.action == 2) { call(options.syntax == 2 ? SCI_REPLACETARGETRE : SCI_REPLACETARGET,replace.size(),reinterpret_cast<LPARAM>(replace.data())); if (call(SCI_GETLENGTH) > 128 * 1024 * 1024) { if (error) *error = L"Replacement output exceeds 128 MiB."; return false; } const auto replacement_end = static_cast<std::uint64_t>(call(SCI_GETTARGETEND)); stop = stop - (last - at) + (replacement_end - at); position = replacement_end; result.changed = true; }
            else position = last;
            if (last == static_cast<std::uint64_t>(at)) { if (position >= stop) break; position = call(SCI_POSITIONAFTER,position); }
            if (progress) progress(static_cast<unsigned>(static_cast<long double>(position - begin) * 100 / (std::max<std::uint64_t>)(1,stop - begin)));
        }
        result.truncated = result.count > result.matches.size() && options.action == 4;
    }
    if (cancel) { if (error) *error = L"Search cancelled; document unchanged."; return false; }
    if (call(SCI_GETSTATUS) == SC_STATUS_WARN_REGEX) { if (error) *error = L"Invalid Scintilla POSIX regular expression. Regex matches stay within a line; capture replacements use \\1 through \\9."; return false; }
    if (result.changed) { const auto size = static_cast<std::size_t>(call(SCI_GETLENGTH)); if (size > 128 * 1024 * 1024) { if (error) *error = L"Replacement output exceeds 128 MiB."; return false; } result.output.resize(size + 1); call(SCI_GETTEXT,result.output.size(),reinterpret_cast<LPARAM>(result.output.data())); result.output.resize(size); }
    if (progress) progress(100); *plan = std::move(result); return true;
}
