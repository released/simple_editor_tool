#include "text_commands.h"
#include "code_editor.h"
#include "../core/line_merge.h"
#include <algorithm>
#include <set>
#include <sstream>
bool TransformEditorLines(const std::string& input,int operation,const std::atomic_bool& cancel,std::string* output,std::wstring* error) {
    using namespace simple_editor::core; if (input.size() > 32 * 1024 * 1024 || operation < 0 || operation > 4) { if (error) *error = L"Line commands accept up to 32 MiB of selected text."; return false; } if (cancel) return false;
    auto lines = SplitLines(CCodeEditor::FromUtf8(input)); if (lines.size() > 200000) { if (error) *error = L"Select at most 200,000 lines for this operation."; return false; } std::wstring ending = L"\r\n"; for (const auto& line : lines) if (!line.ending.empty()) { ending = line.ending; break; } const bool final_newline = !lines.empty() && !lines.back().ending.empty();
    try {
        if (operation <= 1) std::stable_sort(lines.begin(),lines.end(),[&](const auto& a,const auto& b) { if (cancel) throw 1; return operation == 0 ? a.text < b.text : a.text > b.text; });
        else if (operation == 2) { std::set<std::wstring> seen; lines.erase(std::remove_if(lines.begin(),lines.end(),[&](const auto& line) { if (cancel) throw 1; return !seen.insert(line.text).second; }),lines.end()); }
        else if (operation == 3) lines.erase(std::remove_if(lines.begin(),lines.end(),[&](const auto& line) { if (cancel) throw 1; return line.text.find_first_not_of(L" \t") == std::wstring::npos; }),lines.end());
        else for (auto& line : lines) { if (cancel) throw 1; const auto at = line.text.find_last_not_of(L" \t"); line.text.resize(at == std::wstring::npos ? 0 : at + 1); }
    } catch (int) { if (error) *error = L"Line operation cancelled."; return false; }
    std::wstring text; for (std::size_t i = 0; i < lines.size(); ++i) { if (cancel) return false; text += lines[i].text; if (i + 1 < lines.size()) text += lines[i].ending.empty() ? ending : lines[i].ending; else if (final_newline) text += lines[i].ending.empty() ? ending : lines[i].ending; } *output = CCodeEditor::ToUtf8(text); return !cancel;
}
std::string CompleteDocumentWords(const std::string& input,const std::string& prefix,const std::string& keywords,const std::atomic_bool& cancel) {
    if (prefix.empty() || prefix.size() > 128 || input.size() > 32 * 1024 * 1024) return {}; std::set<std::string> words;
    const auto add = [&](const std::string& word) { if (word.size() > prefix.size() && word.size() <= 128 && word.compare(0,prefix.size(),prefix) == 0 && words.size() < 10000) words.insert(word); };
    std::istringstream known(keywords); std::string word; while (known >> word) add(word);
    for (std::size_t at = 0; at < input.size() && !cancel && words.size() < 10000;) { const auto begin = at; while (at < input.size()) { const auto ch = static_cast<unsigned char>(input[at]); if (!(ch >= 128 || ch == '_' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (at > begin && ch >= '0' && ch <= '9'))) break; ++at; } if (at > begin) add(input.substr(begin,at - begin)); else ++at; }
    std::string result; for (const auto& candidate : words) { if (result.size() + candidate.size() > 65536 || cancel) break; if (!result.empty()) result += ' '; result += candidate; } return cancel ? "" : result;
}
