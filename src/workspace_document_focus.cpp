#include "workspace_frame.h"
#include "main_frame.h"
#include "core/text_codec.h"

bool CWorkspaceFrame::NewPastedPair(const std::wstring& left,const std::wstring& right,std::wstring* error) {
    for (const auto* text : {&left,&right}) {
        if (text->size() > 16 * 1024 * 1024 || !simple_editor::core::ValidateText(*text,error) || CCodeEditor::ToUtf8(*text).size() > 16 * 1024 * 1024) { if (error && error->empty()) *error = L"Comparison accepts up to 16 MiB of valid UTF-8 text per side."; return false; }
        std::size_t lines = 1; for (std::size_t i = 0; i < text->size(); ++i) { if ((*text)[i] == L'\r') { ++lines; if (i + 1 < text->size() && (*text)[i + 1] == L'\n') ++i; } else if ((*text)[i] == L'\n') ++lines; if (lines > 200000) { if (error) *error = L"Comparison accepts up to 200,000 lines per side."; return false; } }
    }
    const auto previous = sessions_.size(); NewComparisonInputs(L"",L"",L"",8,false);
    if (sessions_.size() != previous + 1 || !sessions_.back()->advanced_.LoadPastedPair(left,right)) { if (error) *error = L"Could not create a separate pasted-text comparison."; return false; }
    RefreshLabels(); return true;
}

#include "core/syntax_profiles.h"
std::wstring CWorkspaceFrame::SyntaxForPath(const std::wstring& path) const { return simple_editor::core::ResolveSyntax(path,preferences_.syntax_associations); }
std::wstring CWorkspaceFrame::SyntaxAssociations() const { return preferences_.syntax_associations; }
bool CWorkspaceFrame::SetSyntaxAssociations(const std::wstring& text,std::wstring* error) { std::map<std::wstring,std::wstring> mappings; if (!simple_editor::core::ParseSyntaxAssociations(text,&mappings,error)) return false; preferences_.syntax_associations = text; for (auto* frame : sessions_) if (!frame->comparison_session_) frame->ConfigureFileSyntax(); preferences_dirty_ = true; StartPreferenceWrite(); return true; }
