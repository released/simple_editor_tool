#include "text_review.h"
#include <algorithm>
#include <map>
#include <string_view>
namespace simple_editor::core {
namespace {
std::wstring_view Trim(std::wstring_view line) { while (!line.empty() && (line.back() == L' ' || line.back() == L'\t')) line.remove_suffix(1); return line; }
std::wstring HeadingText(std::wstring_view line) { line = Trim(line); while (!line.empty() && (line.front() == L' ' || line.front() == L'\t')) line.remove_prefix(1); auto count = (std::min<std::size_t>)(line.size(),256); if (count && count < line.size() && line[count - 1] >= 0xD800 && line[count - 1] <= 0xDBFF) --count; return std::wstring(line.substr(0,count)); }
}
bool MarkdownOutline(const std::wstring& text,const std::atomic_bool& cancel,std::vector<MarkdownHeading>* output,bool* truncated) {
    if (!output || !truncated || text.size() > 32 * 1024 * 1024) return false;
    std::vector<MarkdownHeading> headings; bool cut = false,fenced = false,comment = false,frontmatter = false; wchar_t fence = 0; std::size_t fence_count = 0,line_number = 0,previous_line = 0; std::wstring previous;
    const auto add = [&](std::wstring title,std::size_t line,unsigned level) { if (title.empty()) title = L"(empty heading)"; if (headings.size() < 4096) headings.push_back({std::move(title),line,level}); else cut = true; };
    for (std::size_t start = 0; start < text.size();) {
        if (cancel) return false; ++line_number; auto end = text.find_first_of(L"\r\n",start); if (end == std::wstring::npos) end = text.size();
        std::wstring_view line(text.data() + start,end - start); start = end; if (start < text.size() && text[start++] == L'\r' && start < text.size() && text[start] == L'\n') ++start;
        unsigned indent = 0; while (!line.empty() && line.front() == L' ') { line.remove_prefix(1); ++indent; } line = Trim(line);
        if (line_number == 1 && line == L"---") { const auto close = text.find(L"\n---",start); if (close != std::wstring::npos && close - start <= 65536 && text.substr(start,close - start).find(L':') != std::wstring::npos) { frontmatter = true; continue; } }
        if (frontmatter) { if (line == L"---" || line == L"...") frontmatter = false; continue; }
        if (comment) { if (line.find(L"-->") != std::wstring_view::npos) comment = false; previous.clear(); continue; }
        if (line.substr(0,4) == L"<!--") { comment = line.find(L"-->") == std::wstring_view::npos; previous.clear(); continue; }
        std::size_t run = 0; if (indent <= 3 && !line.empty() && (line.front() == L'`' || line.front() == L'~')) while (run < line.size() && line[run] == line.front()) ++run;
        if (fenced) { if (run >= fence_count && line.front() == fence && Trim(line.substr(run)).empty()) fenced = false; previous.clear(); continue; }
        if (run >= 3) { fenced = true; fence = line.front(); fence_count = run; previous.clear(); continue; }
        if (line.empty() || indent > 3 || line.front() == L'\t') { previous.clear(); continue; }
        std::size_t hashes = 0; while (hashes < line.size() && line[hashes] == L'#') ++hashes;
        if (hashes && hashes <= 6 && (hashes == line.size() || line[hashes] == L' ' || line[hashes] == L'\t')) {
            auto title = Trim(line.substr(hashes)); std::size_t closing = title.size(); while (closing && title[closing - 1] == L'#') --closing;
            if (closing && closing < title.size() && (title[closing - 1] == L' ' || title[closing - 1] == L'\t')) title = Trim(title.substr(0,closing));
            add(HeadingText(title),line_number,static_cast<unsigned>(hashes)); previous.clear(); continue;
        }
        const bool underline = (line.front() == L'=' || line.front() == L'-') && std::all_of(line.begin(),line.end(),[&](wchar_t c) { return c == line.front(); });
        if (underline) { if (!previous.empty()) add(previous,previous_line,line.front() == L'=' ? 1u : 2u); previous.clear(); continue; }
        if (line.front() == L'>' || line.front() == L'-' || line.front() == L'*' || line.front() == L'+') previous.clear();
        else { previous = HeadingText(line); previous_line = line_number; }
    }
    if (cancel) return false; *output = std::move(headings); *truncated = cut; return true;
}
bool FindMovedBlocks(const TextMergeModel& model,const std::atomic_bool& cancel,std::vector<MovedBlock>* output,bool* truncated,unsigned minimum_lines) {
    if (!output || !truncated || !minimum_lines || model.three_way) return false;
    struct Range { std::size_t start = 0,count = 0; };
    using Key = std::pair<std::uint64_t,std::size_t>;
    std::map<Key,std::vector<Range>> left,right;
    const auto rows = MergeRows(model);
    const auto collect = [&](bool a,std::map<Key,std::vector<Range>>& ranges) {
        const auto& lines = a ? model.left : model.right; Range run;
        const auto emit = [&]() { if (run.count < minimum_lines) return; std::uint64_t hash = 14695981039346656037ULL; bool useful = false;
            for (std::size_t i = run.start; i < run.start + run.count; ++i) { if (cancel) return; for (auto ch : lines[i].text) { hash = (hash ^ ch) * 1099511628211ULL; if (ch != L' ' && ch != L'\t') useful = true; } for (auto ch : lines[i].ending) hash = (hash ^ ch) * 1099511628211ULL; hash = (hash ^ 0x10000) * 1099511628211ULL; }
            if (useful) ranges[{hash,run.count}].push_back(run);
        };
        for (const auto& row : rows) { if (cancel) return; const auto here = a ? row.left : row.right,other = a ? row.right : row.left;
            if (here >= 0 && other < 0) { if (run.count && run.start + run.count != static_cast<std::size_t>(here)) { emit(); run = {}; } if (!run.count) run.start = here; ++run.count; }
            else { emit(); run = {}; }
        } emit();
    };
    collect(true,left); collect(false,right); std::vector<MovedBlock> moves; bool cut = false;
    for (const auto& entry : left) {
        if (cancel) return false; const auto found = right.find(entry.first); if (entry.second.size() != 1 || found == right.end() || found->second.size() != 1) continue;
        const auto& a = entry.second[0]; const auto& b = found->second[0]; bool equal = true;
        for (std::size_t i = 0; i < a.count; ++i) if (model.left[a.start + i].text != model.right[b.start + i].text || model.left[a.start + i].ending != model.right[b.start + i].ending) { equal = false; break; }
        if (equal) { if (moves.size() < 4096) moves.push_back({a.start,b.start,a.count}); else cut = true; }
    }
    if (cancel) return false; *output = std::move(moves); *truncated = cut; return true;
}
}
