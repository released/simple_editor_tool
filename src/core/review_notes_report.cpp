#include "review_notes.h"
#include "stream_output.h"
#include <algorithm>
namespace simple_editor::core {
namespace {
bool Html(StreamOutput& stream,const std::wstring& text,const SaveControl& control,std::wstring* error) {
    for (std::size_t i = 0; i < text.size();) { if (control.cancelled && *control.cancelled) return false; auto end = (std::min)(text.size(),i + 4096); if (end < text.size() && text[end - 1] >= 0xD800 && text[end - 1] <= 0xDBFF) --end; std::wstring escaped; for (; i < end; ++i) escaped += text[i] == L'&' ? L"&amp;" : text[i] == L'<' ? L"&lt;" : text[i] == L'>' ? L"&gt;" : text[i] == L'"' ? L"&quot;" : std::wstring(1,text[i]); if (!stream.Text(escaped,error)) return false; } return true;
}
}
bool ExportReviewedHtml(const TextMergeModel& model,const ComparisonNotes& notes,const std::vector<ProtectedInput>& inputs,const std::filesystem::path& path,const SaveControl& control,std::wstring* error) {
    if (notes.notes.size() > 1024 || model.blocks.size() > 600000 || model.left.size() > 200000 || model.right.size() > 200000 || model.base.size() > 200000) return false;
    for (const auto& block : model.blocks) if (block.left_begin > block.left_end || block.left_end > model.left.size() || block.right_begin > block.right_end || block.right_end > model.right.size() || block.base_begin > block.base_end || block.base_end > model.base.size()) return false;
    const std::atomic_bool idle{false}; const auto& cancel = control.cancelled ? *control.cancelled : idle; std::wstring fingerprint;
    if (!FingerprintComparison(model,cancel,&fingerprint,error) || fingerprint != notes.fingerprint) { if (error) *error = L"Notes belong to different comparison sources or blocks."; return false; }
    for (const auto& note : notes.notes) if (note.first >= model.blocks.size() || !model.blocks[note.first].changed || note.second.text.size() > 4096 || !ValidateText(note.second.text,error)) return false;
    FileStamp expected,current; StreamOutput stream; if (!GuardReviewOutput(path,inputs,&expected,error) || !stream.Open(path,error) || !InspectFile(path,&current,error) || !SameFileState(expected,current)) return false;
    if (!stream.Text(L"<!doctype html><meta charset=\"utf-8\"><title>Comparison review</title><style>body{font:14px sans-serif}table{border-collapse:collapse;width:100%}td,th{border:1px solid #aaa;padding:4px}pre{white-space:pre-wrap;margin:0}.changed{background:#fff2c5}.review{background:#daeafa}</style><h1>Comparison review</h1><p>Review status does not resolve merge conflicts. This report contains original source text and explicitly included annotations.</p><table><tr><th>Base line</th><th>Base</th><th>Left line</th><th>Left</th><th>Right line</th><th>Right</th></tr>",error)) return false;
    for (std::size_t i = 0; i < model.blocks.size(); ++i) {
        if (cancel) return false; const auto& block = model.blocks[i]; const auto note = notes.notes.find(i);
        if (note != notes.notes.end()) { if (!stream.Text(L"<tr class=\"review\"><td colspan=\"6\"><strong>Block " + std::to_wstring(i + 1) + (note->second.reviewed ? L" — Reviewed" : L" — Pending review") + L"</strong><pre>",error) || !Html(stream,note->second.text,control,error) || !stream.Text(L"</pre></td></tr>",error)) return false; }
        const auto rows = (std::max)({block.left_end - block.left_begin,block.right_end - block.right_begin,model.three_way ? block.base_end - block.base_begin : 0});
        for (std::size_t row = 0; row < rows; ++row) { if (cancel || !stream.Text(block.changed ? L"<tr class=\"changed\">" : L"<tr>",error)) return false;
            const auto cell = [&](const std::vector<TextLine>& lines,std::size_t begin,std::size_t end,bool enabled) { if (!enabled || row >= end - begin) return stream.Text(L"<td></td><td></td>",error); const auto at = begin + row; return at < lines.size() && stream.Text(L"<td>" + std::to_wstring(at + 1) + L"</td><td><pre>",error) && Html(stream,lines[at].text + lines[at].ending,control,error) && stream.Text(L"</pre></td>",error); };
            if (!cell(model.base,block.base_begin,block.base_end,model.three_way) || !cell(model.left,block.left_begin,block.left_end,true) || !cell(model.right,block.right_begin,block.right_end,true) || !stream.Text(L"</tr>",error)) return false;
        } if (control.progress) control.progress(static_cast<unsigned>((i + 1) * 100 / (std::max<std::size_t>)(1,model.blocks.size())));
    }
    return stream.Text(L"</table>",error) && stream.Commit(control,error);
}
}
