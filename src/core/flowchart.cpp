#include "flowchart.h"
#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <stdexcept>
namespace simple_editor::core {
namespace {
class Parser {
public:
    Parser(const std::wstring& input,const std::atomic_bool& cancel) : text_(input),cancel_(cancel) {}
    Flowchart Run() {
        Space(); const auto kind = Word(); if (kind != L"flowchart" && kind != L"graph") Fail();
        Space(); const auto direction = Word();
        if (direction == L"TD" || direction == L"TB") graph_.direction = FlowDirection::Down;
        else if (direction == L"BT") graph_.direction = FlowDirection::Up;
        else if (direction == L"LR") graph_.direction = FlowDirection::Right;
        else if (direction == L"RL") graph_.direction = FlowDirection::Left;
        else Fail();
        End();
        while (at_ < text_.size()) {
            Space(); if (at_ == text_.size()) break;
            auto from = Node(); Space(false);
            while (Starts(L"-->")) {
                at_ += 3; Space(false); std::wstring label;
                if (Starts(L"|")) { ++at_; label = Label(L"|"); Space(false); }
                const auto to = Node(); if (graph_.edges.size() == 96) Fail();
                graph_.edges.push_back({from,to,std::move(label)}); from = to; Space(false);
            }
            End();
        }
        if (graph_.nodes.empty()) Fail();
        return std::move(graph_);
    }
private:
    [[noreturn]] static void Fail() { throw std::invalid_argument("unsupported flowchart"); }
    bool Starts(const wchar_t* token) const { return text_.compare(at_,std::wcslen(token),token) == 0; }
    void Space(bool newline = true) {
        while (at_ < text_.size()) {
            if (cancel_) Fail();
            if (text_[at_] == L' ' || text_[at_] == L'\t' || (newline && (text_[at_] == L'\r' || text_[at_] == L'\n' || text_[at_] == L';'))) { ++at_; continue; }
            if (Starts(L"%%{")) Fail();
            if (Starts(L"%%")) { while (at_ < text_.size() && text_[at_] != L'\r' && text_[at_] != L'\n') ++at_; continue; }
            break;
        }
    }
    void End() {
        Space(false);
        if (at_ < text_.size() && text_[at_] != L'\r' && text_[at_] != L'\n' && text_[at_] != L';') Fail();
        Space();
    }
    std::wstring Word() {
        const auto begin = at_;
        while (at_ < text_.size() && ((text_[at_] >= L'a' && text_[at_] <= L'z') || (text_[at_] >= L'A' && text_[at_] <= L'Z') ||
            (text_[at_] >= L'0' && text_[at_] <= L'9') || text_[at_] == L'_')) ++at_;
        if (at_ == begin || at_ - begin > 64) Fail();
        return text_.substr(begin,at_ - begin);
    }
    std::wstring Label(const wchar_t* end) {
        std::wstring value; bool quoted = Starts(L"\""); if (quoted) ++at_;
        bool closed = !quoted;
        while (at_ < text_.size()) {
            if (cancel_ || value.size() > 256) Fail();
            if (quoted && text_[at_] == L'"') { ++at_; closed = true; break; }
            if (!quoted && Starts(end)) break;
            const auto c = text_[at_++];
            if (c == L'\r' || c == L'\n' || c < 32) Fail();
            if (c == L'<' && (text_.compare(at_ - 1,4,L"<br>") == 0 || text_.compare(at_ - 1,5,L"<br/>") == 0 || text_.compare(at_ - 1,6,L"<br />") == 0)) {
                while (at_ < text_.size() && text_[at_++] != L'>') {} value += L'\n';
            } else if (c == L'<' || c == L'>' || c == L'`' || c == L'&') Fail();
            else value += c;
        }
        if (!closed || value.size() > 256 || !Starts(end)) Fail(); at_ += std::wcslen(end);
        if (value.empty() || value.find_first_not_of(L" \t\n") == std::wstring::npos) Fail();
        return value;
    }
    std::size_t Node() {
        const auto id = Word();
        // Reserved statements must never become plausible-looking graph nodes.
        if (id == L"subgraph" || id == L"end" || id == L"style" || id == L"classDef" || id == L"class" || id == L"click" || id == L"direction") Fail();
        Space(false); std::wstring label; auto shape = FlowShape::Rectangle; bool explicit_label = false;
        if (Starts(L"[[") || Starts(L"[(") || Starts(L"([") || Starts(L"{{") || Starts(L"(((") || Starts(L"[/") || Starts(L"[\\")) Fail();
        if (Starts(L"[")) { ++at_; label = Label(L"]"); explicit_label = true; }
        else if (Starts(L"((")) { at_ += 2; label = Label(L"))"); shape = FlowShape::Circle; explicit_label = true; }
        else if (Starts(L"(")) { ++at_; label = Label(L")"); shape = FlowShape::Rounded; explicit_label = true; }
        else if (Starts(L"{")) { ++at_; label = Label(L"}"); shape = FlowShape::Diamond; explicit_label = true; }
        const auto found = std::find_if(graph_.nodes.begin(),graph_.nodes.end(),[&](const auto& node) { return node.id == id; });
        if (found != graph_.nodes.end()) { if (explicit_label) { found->label = label; found->shape = shape; } return static_cast<std::size_t>(found - graph_.nodes.begin()); }
        if (graph_.nodes.size() == 64) Fail();
        graph_.nodes.push_back({id,explicit_label ? label : id,shape}); return graph_.nodes.size() - 1;
    }
    const std::wstring& text_; const std::atomic_bool& cancel_; std::size_t at_ = 0; Flowchart graph_;
};
}
bool ParseFlowchart(const std::wstring& input,Flowchart& output,const std::atomic_bool& cancel) {
    output = {}; if (input.size() > 16384 || cancel) return false;
    try { auto parsed = Parser(input,cancel).Run(); if (cancel) return false; output = std::move(parsed); return true; }
    catch (const std::invalid_argument&) { return false; }
}
}
