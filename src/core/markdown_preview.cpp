#include "markdown_preview.h"
#include <algorithm>
#include <cstdint>
#include <vector>
namespace simple_editor::core {
namespace {
std::string Escape(const std::wstring& text,std::wstring* visible) {
    std::string out;
    for (const auto c : text) {
        if (visible && c > 32) visible->push_back(c);
        if (c == L'\\' || c == L'{' || c == L'}') { out += '\\'; out += static_cast<char>(c); }
        else if (c == L'\t') out += "\\tab ";
        else if (c >= 32 && c < 127) out += static_cast<char>(c);
        else if (c >= 32) out += "\\u" + std::to_string(static_cast<std::int16_t>(c)) + "?";
    }
    return out;
}
std::string Inline(const std::wstring& text,std::wstring* visible) {
    std::string out;
    bool links_possible = true;
    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == L'\\' && i + 1 < text.size() && std::wstring(L"\\`*_{}[]()#+-.!>~|\"").find(text[i + 1]) != std::wstring::npos) { out += Escape(text.substr(i + 1,1),visible); i += 2; continue; }
        if (text[i] == L'`') {
            const auto end = text.find(L'`',i + 1);
            if (end != std::wstring::npos) { out += "{\\f1\\cf4 " + Escape(text.substr(i + 1,end - i - 1),visible) + "}"; i = end + 1; continue; }
        }
        const bool image = text[i] == L'!' && i + 1 < text.size() && text[i + 1] == L'[';
        if (links_possible && (text[i] == L'[' || image)) {
            const auto start = i + (image ? 2 : 1),middle = text.find(L"](",start);
            const auto end = middle == std::wstring::npos ? middle : text.find(L')',middle + 2);
            if (end != std::wstring::npos) {
                out += "{\\cf3\\ul " + Escape(text.substr(start,middle - start),visible) + "}";
                if (image) out += " {\\cf6 " + Escape(L"[" + text.substr(middle + 2,end - middle - 2) + L"]",visible) + "}";
                i = end + 1; continue;
            }
            links_possible = false;
        }
        const auto marker = text.substr(i,2);
        if (marker == L"**" || marker == L"__" || marker == L"~~") {
            const auto end = text.find(marker,i + 2);
            if (end != std::wstring::npos) { out += std::string(marker == L"~~" ? "{\\strike " : "{\\b ") + Escape(text.substr(i + 2,end - i - 2),visible) + "}"; i = end + 2; continue; }
        }
        if (text[i] == L'*' || text[i] == L'_') {
            const auto end = text.find(text[i],i + 1);
            if (end != std::wstring::npos) { out += "{\\i " + Escape(text.substr(i + 1,end - i - 1),visible) + "}"; i = end + 1; continue; }
        }
        out += Escape(text.substr(i++,1),visible);
    }
    return out;
}
std::wstring Trim(const std::wstring& s) { const auto a = s.find_first_not_of(L" \t\r"); return a == std::wstring::npos ? L"" : s.substr(a,s.find_last_not_of(L" \t\r") - a + 1); }
std::vector<std::wstring> Cells(std::wstring line) {
    line = Trim(line); if (!line.empty() && line.front() == L'|') line.erase(0,1); if (!line.empty() && line.back() == L'|') line.pop_back();
    std::vector<std::wstring> cells; std::wstring cell; bool escaped = false;
    for (const auto c : line) { if (c == L'|' && !escaped) { cells.push_back(Trim(cell)); cell.clear(); } else cell += c; escaped = c == L'\\' && !escaped; }
    cells.push_back(Trim(cell)); return cells;
}
bool TableSeparator(const std::wstring& line) {
    const auto cells = Cells(line); if (cells.size() < 2 || cells.size() > 12) return false;
    for (const auto& cell : cells) { if (cell.find(L"--") == std::wstring::npos || cell.find_first_not_of(L" -:\t") != std::wstring::npos) return false; } return true;
}
bool Numbered(const std::wstring& text) {
    const auto end = text.find_first_not_of(L"0123456789");
    return end && end != std::wstring::npos && end + 1 < text.size() && (text[end] == L'.' || text[end] == L')') && text[end + 1] == L' ';
}
bool BlockStart(const std::wstring& text) {
    if (text.empty()) return true;
    if (text.rfind(L"```",0) == 0 || text.rfind(L"~~~",0) == 0 || text[0] == L'>' || Numbered(text)) return true;
    if (text.rfind(L"- ",0) == 0 || text.rfind(L"* ",0) == 0 || text.rfind(L"+ ",0) == 0 || text == L"---" || text == L"***" || text == L"___") return true;
    std::size_t n = 0; while (n < text.size() && text[n] == L'#') ++n;
    return n && n <= 6 && n < text.size() && text[n] == L' ';
}
}
MarkdownPreview RenderMarkdownPreview(const std::wstring& input,bool dark,unsigned half_points,int width,const std::wstring& notice,const std::atomic_bool& cancel,
    const MarkdownDiagramRenderer& diagram,const std::wstring& diagram_notice) {
    MarkdownPreview result; auto count = (std::min<std::size_t>)(input.size(),65536);
    if (count && count < input.size() && input[count - 1] >= 0xD800 && input[count - 1] <= 0xDBFF && input[count] >= 0xDC00 && input[count] <= 0xDFFF) --count;
    result.truncated = input.size() > count;
    std::vector<std::wstring> lines; std::size_t at = 0;
    while (lines.size() < 4096 && at < count) {
        auto end = at; while (end < count && input[end] != L'\r' && input[end] != L'\n') ++end;
        lines.push_back(input.substr(at,end - at)); at = end;
        if (at < count) { const auto delimiter = input[at++]; if (delimiter == L'\r' && at < count && input[at] == L'\n') ++at; }
    }
    if (at < count) result.truncated = true;
    result.source_lines = lines.size(); result.anchors.reserve(lines.size()); result.scroll_text.reserve(count);
    const auto mark = [&](std::size_t source_line) { if (result.anchors.empty() || result.anchors.back().source_line != source_line) result.anchors.push_back({source_line,result.scroll_text.size()}); };
    half_points = (std::clamp)(half_points,12u,120u); width = (std::clamp)(width,1200,24000);
    result.rtf = "{\\rtf1\\ansi\\deff0\\uc1{\\fonttbl{\\f0 Segoe UI;}{\\f1 Consolas;}}{\\colortbl;";
    result.rtf += dark ? "\\red229\\green229\\blue229;\\red63\\green63\\blue63;\\red122\\green162\\blue247;\\red229\\green192\\blue123;\\red239\\green189\\blue139;\\red160\\green170\\blue160;}" : "\\red30\\green30\\blue30;\\red255\\green255\\blue255;\\red30\\green80\\blue160;\\red135\\green90\\blue10;\\red150\\green70\\blue10;\\red80\\green105\\blue80;}";
    const auto normal = "\\pard\\plain\\cf1\\f0\\fs" + std::to_string(half_points) + "\\sa140\\sl280\\slmult1 ";
    std::wstring fence; unsigned diagrams = 0;
    for (std::size_t n = 0; n < lines.size(); ++n) {
        if (cancel) { result.rtf.clear(); return result; }
        mark(n);
        const auto trimmed = Trim(lines[n]);
        if (trimmed.rfind(L"```",0) == 0 || trimmed.rfind(L"~~~",0) == 0) {
            if (fence.empty() && Trim(trimmed.substr(3)) == L"mermaid") {
                auto end = n + 1; std::wstring block;
                for (; end < lines.size() && Trim(lines[end]) != trimmed.substr(0,3); ++end) {
                    if (cancel) { result.rtf.clear(); return result; }
                    if (block.size() <= 16384) block += lines[end] + L'\n';
                }
                const auto picture = diagram && end < lines.size() && diagrams < 8 && block.size() <= 16384 ? diagram(block,dark,half_points,width,cancel) : std::string{};
                if (cancel) { result.rtf.clear(); return result; }
                if (!picture.empty()) {
                    for (auto line = n + 1; line <= end; ++line) mark(line);
                    result.scroll_text += L'\xFFFC'; result.rtf += normal + "\\qc " + picture + "\\par "; ++diagrams;
                    n = end; continue;
                }
                if (!diagram_notice.empty()) result.rtf += normal + "\\cf4 " + Escape(diagram_notice,&result.scroll_text) + "\\par ";
            }
            const auto delimiter = trimmed.substr(0,3); if (fence.empty()) fence = delimiter; else if (delimiter == fence) fence.clear(); else result.rtf += normal + "{\\f1\\cf4 " + Escape(lines[n],&result.scroll_text) + "}\\par "; continue;
        }
        if (!fence.empty()) { result.rtf += normal + "\\sa0\\sl240 {\\f1\\cf4 " + Escape(lines[n],&result.scroll_text) + "}\\par "; continue; }
        if (trimmed.empty()) continue;
        if (n + 1 < lines.size() && lines[n].find(L'|') != std::wstring::npos && TableSeparator(lines[n + 1])) {
            const auto columns = Cells(lines[n]).size();
            if (columns >= 2 && columns <= 12) {
                bool header = true;
                for (; n < lines.size() && lines[n].find(L'|') != std::wstring::npos; ++n) {
                    mark(n);
                    if (TableSeparator(lines[n])) continue;
                    const auto cells = Cells(lines[n]); if (cells.size() != columns) break;
                    result.rtf += "\\trowd\\trgaph70";
                    for (std::size_t c = 0; c < columns; ++c) result.rtf += "\\clbrdrb\\brdrs\\brdrw8\\brdrcf6\\cellx" + std::to_string((c + 1) * width / columns);
                    result.rtf += "\\pard\\intbl\\f0\\fs" + std::to_string(half_points) + (header ? "\\b\\cf4 " : "\\b0\\cf1 ");
                    for (const auto& cell : cells) result.rtf += Inline(cell,&result.scroll_text) + "\\cell ";
                    result.rtf += "\\row "; header = false;
                }
                result.rtf += normal + "\\par "; if (n) --n; continue;
            }
        }
        result.rtf += normal;
        std::size_t heading = 0; while (heading < trimmed.size() && trimmed[heading] == L'#') ++heading;
        if (heading && heading <= 6 && heading < trimmed.size() && trimmed[heading] == L' ') {
            result.rtf += "\\sb180\\b\\cf5\\fs" + std::to_string(half_points + static_cast<unsigned>((7 - heading) * 3)) + " " + Inline(Trim(trimmed.substr(heading)),&result.scroll_text) + "\\b0\\par ";
        } else if (trimmed.rfind(L">",0) == 0) result.rtf += "\\li280\\cf6 " + Inline(Trim(trimmed.substr(1)),&result.scroll_text) + "\\par ";
        else if (trimmed.rfind(L"- ",0) == 0 || trimmed.rfind(L"* ",0) == 0 || trimmed.rfind(L"+ ",0) == 0) { result.scroll_text += L'\x2022'; result.rtf += "\\li260\\fi-180 \\u8226?\\tab " + Inline(trimmed.substr(2),&result.scroll_text) + "\\par "; }
        else if (trimmed == L"---" || trimmed == L"***" || trimmed == L"___") result.rtf += "\\brdrb\\brdrs\\brdrw10 \\par ";
        else if (Numbered(trimmed)) result.rtf += Inline(trimmed,&result.scroll_text) + "\\par ";
        else {
            for (;;) {
                mark(n);
                auto body = lines[n];
                const bool hard = (body.size() >= 2 && body.substr(body.size() - 2) == L"  ") || (!body.empty() && body.back() == L'\\');
                if (hard) body.erase(body.size() - (body.back() == L'\\' ? 1 : 2));
                result.rtf += Inline(body,&result.scroll_text);
                if (n + 1 == lines.size() || BlockStart(Trim(lines[n + 1])) || (n + 2 < lines.size() && lines[n + 1].find(L'|') != std::wstring::npos && TableSeparator(lines[n + 2]))) break;
                result.rtf += hard ? "\\line " : " "; ++n;
            }
            result.rtf += "\\par ";
        }
    }
    if (result.truncated) result.rtf += normal + "\\cf4 " + Escape(notice,&result.scroll_text) + "\\par ";
    result.rtf += "}"; return result;
}
}
