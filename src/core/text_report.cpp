#include "text_report.h"
#include <bcrypt.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace simple_editor::core {
namespace {
std::wstring Canonical(const std::wstring& text) { std::wstring result; result.reserve(text.size()); for (const auto& line : SplitLines(text)) { result += line.text; if (!line.ending.empty()) result += L'\n'; } return result; }
std::wstring Name(const std::wstring& path) { auto name = std::filesystem::path(path).filename().wstring(); if (name.empty()) name = L"input"; for (auto& ch : name) if (ch < 32 || ch == L'\t') ch = L'_'; return name; }
bool Stop(const std::atomic_bool& cancel,std::wstring* error) { if (cancel) { if (error) *error = L"Text report/patch cancelled; existing output retained."; return true; } return false; }
bool Hash(const std::wstring& text,std::wstring* result,std::wstring* error) {
    std::vector<std::uint8_t> bytes; TextCodec utf8; if (!EncodeText(text,utf8,false,&bytes,error)) return false;
    std::vector<unsigned char> object; struct Provider { BCRYPT_ALG_HANDLE value = nullptr; BCRYPT_HASH_HANDLE hash = nullptr; ~Provider() { if (hash) BCryptDestroyHash(hash); if (value) BCryptCloseAlgorithmProvider(value,0); } } provider;
    unsigned char digest[32]{}; DWORD length = 0,returned = 0;
    if (BCryptOpenAlgorithmProvider(&provider.value,BCRYPT_SHA256_ALGORITHM,nullptr,0) < 0 || BCryptGetProperty(provider.value,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&length),sizeof(length),&returned,0) < 0) { if (error) *error = L"Could not initialize the normalized source hash."; return false; }
    object.resize(length);
    if (BCryptCreateHash(provider.value,&provider.hash,object.data(),length,nullptr,0,0) < 0 || (!bytes.empty() && BCryptHashData(provider.hash,bytes.data(),static_cast<ULONG>(bytes.size()),0) < 0) || BCryptFinishHash(provider.hash,digest,32,0) < 0) { if (error) *error = L"Could not compute the normalized source hash."; return false; }
    std::wostringstream out; out << std::uppercase << std::hex << std::setfill(L'0'); for (auto value : digest) out << std::setw(2) << static_cast<unsigned>(value); *result = out.str(); return true;
}
bool OpenReport(const std::filesystem::path& path,const std::vector<ProtectedInput>& inputs,StreamOutput* stream,std::wstring* error,bool require_new) {
    FileStamp target; if (!InspectFile(path,&target,error)) return false; std::error_code code; const auto output = std::filesystem::absolute(path,code).lexically_normal(); if (code) return false;
    for (const auto& input : inputs) { const auto source = std::filesystem::absolute(input.path,code).lexically_normal(); if (code || _wcsicmp(source.c_str(),output.c_str()) == 0 || (target.exists && input.identity != std::pair<DWORD,std::uint64_t>{} && input.identity == std::make_pair(target.volume,target.identity))) { if (error) *error = L"A text report needs an output separate from every input."; return false; } }
    return stream->Open(path,error,require_new);
}
bool Html(StreamOutput& stream,const std::wstring& text,const std::atomic_bool& cancel,std::wstring* error) {
    for (std::size_t at = 0; at < text.size();) { if (Stop(cancel,error)) return false; auto end = (std::min)(text.size(),at + 4096); if (end < text.size() && text[end - 1] >= 0xD800 && text[end - 1] <= 0xDBFF && text[end] >= 0xDC00 && text[end] <= 0xDFFF) --end; std::wstring escaped; escaped.reserve((end - at) * 2);
        for (auto i = at; i < end; ++i) { const auto ch = text[i]; escaped += ch == L'&' ? L"&amp;" : ch == L'<' ? L"&lt;" : ch == L'>' ? L"&gt;" : ch == L'"' ? L"&quot;" : std::wstring(1,ch); }
        if (!stream.Text(escaped,error)) return false; at = end;
    }
    return true;
}
bool Range(const std::wstring& text,std::size_t* begin,std::size_t* count) {
    const auto comma = text.find(L','); const auto first = text.substr(0,comma), second = comma == std::wstring::npos ? L"1" : text.substr(comma + 1); if (first.empty() || second.empty() || first.find_first_not_of(L"0123456789") != std::wstring::npos || second.find_first_not_of(L"0123456789") != std::wstring::npos) return false;
    try { const auto a = std::stoull(first), n = std::stoull(second); if (a > 200000 || n > 200000 || (n && !a)) return false; *begin = static_cast<std::size_t>(a - (n != 0)); *count = static_cast<std::size_t>(n); return true; } catch (...) { return false; }
}
}
bool BuildUnifiedPatch(const std::wstring& left,const std::wstring& right,const std::wstring& left_name,const std::wstring& right_name,const std::atomic_bool& cancel,std::wstring* patch,TextReportStats* statistics,std::wstring* error) {
    if (!ValidateText(left,error) || !ValidateText(right,error)) return false;
    if (left.size() > 32 * 1024 * 1024 || right.size() > 32 * 1024 * 1024 || Stop(cancel,error)) { if (error && error->empty()) *error = L"Patch inputs exceed 32 million UTF-16 units."; return false; }
    const auto a = Canonical(left), b = Canonical(right); TextMergeModel model; if (!BuildTextMerge(L"",a,b,false,{},cancel,&model,error)) return false;
    struct Operation { wchar_t kind; const TextLine* line; }; std::vector<Operation> operations; TextReportStats stats;
    for (const auto& block : model.blocks) { if (Stop(cancel,error)) return false; if (block.changed) { ++stats.blocks; stats.removed += block.left_end - block.left_begin; stats.added += block.right_end - block.right_begin; for (auto i = block.left_begin; i < block.left_end; ++i) operations.push_back({L'-',&model.left[i]}); for (auto i = block.right_begin; i < block.right_end; ++i) operations.push_back({L'+',&model.right[i]}); }
        else { stats.equal += block.left_end - block.left_begin; for (auto i = block.left_begin; i < block.left_end; ++i) operations.push_back({L' ',&model.left[i]}); } }
    std::wstring hash; if (!Hash(a,&hash,error)) return false;
    std::wstring result = L"# SimpleEditorTool normalized-sha256: " + hash + L"\n--- a/" + Name(left_name) + L"\n+++ b/" + Name(right_name) + L"\n";
    std::vector<std::size_t> old(operations.size() + 1), updated(operations.size() + 1); for (std::size_t i = 0; i < operations.size(); ++i) { old[i + 1] = old[i] + (operations[i].kind != L'+'); updated[i + 1] = updated[i] + (operations[i].kind != L'-'); }
    for (std::size_t at = 0; at < operations.size();) { if (Stop(cancel,error)) return false; while (at < operations.size() && operations[at].kind == L' ') ++at; if (at == operations.size()) break;
        const auto begin = at > 3 ? at - 3 : 0; auto end = (std::min)(operations.size(),at + 4); std::size_t next = at + 1;
        for (;;) { while (next < operations.size() && operations[next].kind == L' ') ++next; if (next == operations.size() || next > end + 3) break; end = (std::min)(operations.size(),next + 4); ++next; }
        const auto old_count = old[end] - old[begin], new_count = updated[end] - updated[begin]; ++stats.hunks;
        result += L"@@ -" + std::to_wstring(old[begin] + (old_count != 0)) + L"," + std::to_wstring(old_count) + L" +" + std::to_wstring(updated[begin] + (new_count != 0)) + L"," + std::to_wstring(new_count) + L" @@\n";
        for (auto i = begin; i < end; ++i) { result += operations[i].kind; result += operations[i].line->text; result += L'\n'; if (operations[i].line->ending.empty()) result += L"\\ No newline at end of file\n"; } at = end;
    }
    if (Stop(cancel,error)) return false; if (statistics) *statistics = stats; *patch = std::move(result); return true;
}
bool ApplyUnifiedPatch(const std::wstring& source,const std::wstring& patch,const std::atomic_bool& cancel,std::wstring* output,TextReportStats* statistics,std::wstring* error) {
    if (!ValidateText(source,error) || !ValidateText(patch,error)) return false;
    const auto fail = [&](const std::wstring& reason) { if (error) *error = reason; return false; };
    if (source.size() > 32 * 1024 * 1024 || patch.size() > 64 * 1024 * 1024 || Stop(cancel,error)) return fail(L"Source/patch exceeds the supported text limit or was cancelled.");
    const auto original = SplitLines(source), lines = SplitLines(patch); if (original.size() > 200000 || lines.size() > 1000000) return fail(L"Source/patch has too many lines."); std::size_t at = 0;
    const std::wstring hash_prefix = L"# SimpleEditorTool normalized-sha256: ";
    if (!lines.empty() && lines[0].text.find(hash_prefix) == 0) { std::wstring hash; if (!Hash(Canonical(source),&hash,error)) return false; if (lines[0].text.substr(hash_prefix.size()) != hash) return fail(L"Source differs from the patch's normalized SHA-256; no changes applied."); ++at; }
    if (at + 2 > lines.size() || lines[at].text.find(L"--- ") != 0 || lines[at + 1].text.find(L"+++ ") != 0) return fail(L"Expected one unified patch file header.");
    const bool create = lines[at].text == L"--- /dev/null", remove = lines[at + 1].text == L"+++ /dev/null"; if (create && !original.empty()) return fail(L"Creation patch requires an empty source buffer."); at += 2;
    std::wstring ending = L"\n"; for (const auto& line : original) if (!line.ending.empty()) { ending = line.ending; break; } std::vector<TextLine> result; std::size_t position = 0; TextReportStats stats;
    while (at < lines.size()) {
        if (Stop(cancel,error)) return false; const auto& header = lines[at++].text; const auto middle = header.find(L" +"), end = middle == std::wstring::npos ? middle : header.find(L" @@",middle + 2); std::size_t old_start = 0,old_count = 0,new_start = 0,new_count = 0;
        if (header.find(L"@@ -") != 0 || end == std::wstring::npos || !Range(header.substr(4,middle - 4),&old_start,&old_count) || !Range(header.substr(middle + 2,end - middle - 2),&new_start,&new_count) || old_start < position || old_start > original.size()) return fail(L"Invalid, overlapping or reordered unified hunk.");
        result.insert(result.end(),original.begin() + position,original.begin() + old_start); position = old_start; if (new_start != result.size()) return fail(L"New-side hunk position does not match the validated source."); ++stats.hunks; ++stats.blocks; std::size_t consumed = 0, produced = 0;
        while (at < lines.size() && lines[at].text.find(L"@@ ") != 0) {
            const auto& line = lines[at++].text; if (line.empty() || (line[0] != L' ' && line[0] != L'-' && line[0] != L'+')) return fail(L"Unsupported patch data or multiple-file patch."); const auto kind = line[0]; const auto value = line.substr(1); bool no_eol = false;
            if (at < lines.size() && lines[at].text == L"\\ No newline at end of file") { no_eol = true; ++at; }
            if (kind != L'+') { if (position >= original.size() || original[position].text != value || original[position].ending.empty() != no_eol) return fail(L"Patch context does not match source line " + std::to_wstring(position + 1) + L"; no changes applied."); if (kind == L' ') { result.push_back(original[position]); ++stats.equal; } else ++stats.removed; ++position; ++consumed; }
            if (kind != L'-') ++produced;
            if (kind == L'+') { result.push_back({value,no_eol ? L"" : ending}); ++stats.added; }
            if (consumed > old_count || produced > new_count || result.size() > 200000) return fail(L"Hunk counts or output line limit exceeded.");
        }
        if (consumed != old_count || produced != new_count) return fail(L"Unified hunk counts do not match its data.");
    }
    result.insert(result.end(),original.begin() + position,original.end()); if (remove && !result.empty()) return fail(L"Deletion patch must result in an empty buffer.");
    std::wstring text; for (std::size_t i = 0; i < result.size(); ++i) { if (i + 1 < result.size() && result[i].ending.empty()) return fail(L"Unterminated line appears before additional patch output."); text += result[i].text; text += result[i].ending; if (text.size() > 64 * 1024 * 1024) return fail(L"Patch output exceeds its text limit."); }
    if (!source.empty()) text = TextFromEditControl(source,TextForEditControl(text));
    if (Stop(cancel,error)) return false; *output = std::move(text); if (statistics) *statistics = stats; return true;
}
bool ExportTextReport(const std::wstring& left,const std::wstring& right,const std::vector<ProtectedInput>& inputs,const std::filesystem::path& path,TextReportFormat format,const SaveControl& control,TextReportStats* statistics,std::wstring* error,bool require_new,const LineOptions* options,const std::wstring& rule_note) {
    if (!ValidateText(left,error) || !ValidateText(right,error)) return false;
    std::atomic_bool never{false}; const auto& cancel = control.cancelled ? *control.cancelled : never; if (Stop(cancel,error)) return false; StreamOutput stream; if (!OpenReport(path,inputs,&stream,error,require_new)) return false; TextReportStats stats;
    if (format == TextReportFormat::UnifiedPatch) { std::wstring patch; if (!BuildUnifiedPatch(left,right,inputs.empty() ? L"left" : inputs[0].path.wstring(),inputs.size() < 2 ? L"right" : inputs[1].path.wstring(),cancel,&patch,&stats,error) || !stream.Text(patch,error)) return false; }
    else {
        TextMergeModel model; if (!BuildTextMerge(L"",left,right,false,options ? *options : LineOptions{},cancel,&model,error)) return false;
        for (const auto& block : model.blocks) { if (block.changed && block.important) { ++stats.blocks; stats.added += block.right_end - block.right_begin; stats.removed += block.left_end - block.left_begin; } else if (block.changed) ++stats.ignored; else stats.equal += block.left_end - block.left_begin; }
        const auto summary = (inputs.empty() ? L"" : L"Left: " + inputs[0].path.wstring() + L"\r\n") + (inputs.size() < 2 ? L"" : L"Right: " + inputs[1].path.wstring() + L"\r\n") + L"Changed blocks: " + std::to_wstring(stats.blocks) + L"\r\nAdded/replaced right lines: " + std::to_wstring(stats.added) + L"\r\nRemoved/replaced left lines: " + std::to_wstring(stats.removed) + L"\r\nEqual lines: " + std::to_wstring(stats.equal) + L"\r\n";
        const auto annotated = summary + (options ? L"Ignored difference blocks: " + std::to_wstring(stats.ignored) + L"\r\nComparison rules:\r\n" + rule_note + L"\r\nRaw input text is retained below. Unified patches always include raw differences.\r\n" : L"");
        if (format == TextReportFormat::Summary) { if (!stream.Text(annotated,error)) return false; }
        else { if (!stream.Text(L"<!doctype html><meta charset=\"utf-8\"><title>Text comparison</title><style>body{font:14px Consolas,monospace}table{border-collapse:collapse;width:100%}td,th{border:1px solid #bbb;padding:4px;vertical-align:top}pre{white-space:pre-wrap;margin:0}.change{background:#fff3c0}</style><h1>Text comparison</h1><pre>",error) || !Html(stream,annotated,cancel,error) || !stream.Text(L"</pre><table><tr><th>Left line</th><th>Left</th><th>EOL</th><th>Right line</th><th>Right</th><th>EOL</th></tr>",error)) return false;
            const auto rows = MergeRows(model); for (std::size_t i = 0; i < rows.size(); ++i) { if (Stop(cancel,error)) return false; const auto& row = rows[i]; if (!stream.Text(model.blocks[row.block].changed && model.blocks[row.block].important ? L"<tr class=change>" : L"<tr>",error)) return false;
                for (unsigned side = 0; side < 2; ++side) { const auto number = side ? row.right : row.left; if (!stream.Text(L"<td>" + (number < 0 ? L"" : std::to_wstring(number + 1)) + L"</td><td><pre>",error)) return false; if (number >= 0 && !Html(stream,(side ? model.right : model.left)[number].text,cancel,error)) return false; const auto ending = number < 0 ? std::wstring() : (side ? model.right : model.left)[number].ending; const auto label = number < 0 ? L"" : ending == L"\r\n" ? L"CRLF" : ending == L"\n" ? L"LF" : ending == L"\r" ? L"CR" : L"EOF"; if (!stream.Text(L"</pre></td><td>" + std::wstring(label) + L"</td>",error)) return false; }
                if (!stream.Text(L"</tr>\n",error)) return false; if (control.progress && (i & 255) == 0) control.progress(static_cast<unsigned>(i * 80 / (std::max<std::size_t>)(1,rows.size())));
            } if (!stream.Text(L"</table>",error)) return false;
        }
    }
    if (Stop(cancel,error)) return false; SaveControl commit = control; commit.progress = [&](unsigned p) { if (control.progress) control.progress(80 + p / 5); }; if (!stream.Commit(commit,error)) return false; if (statistics) *statistics = stats; return true;
}
}
