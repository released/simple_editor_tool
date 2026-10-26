#include "main_frame.h"
#include "ui/localization.h"
#include "resource.h"
#include "core/compare_presets.h"
#include "ui/feature_dialogs.h"
#include <algorithm>
using namespace simple_editor::core;
void CMainFrame::PresetCommand(UINT command) {
    if (!AdvancedActive() || displayed_mode_ == 8 || IsBusy()) { status_.SetPaneText(0,L"Choose a file/folder Text, three-way, CSV/TSV or JSON comparison first."); return; }
    if (preset_path_.empty()) { wchar_t exe[32768]{}; GetModuleFileNameW(nullptr,exe,32768); preset_path_ = (std::filesystem::path(exe).parent_path() / L"comparison_presets.ini").wstring(); }
    std::vector<ComparePreset> presets; std::wstring error;
    if (!LoadComparePresets(preset_path_,&presets,&error)) { status_.SetPaneText(0,error.c_str()); return; }
    if (command == ID_WORKFLOW_PRESET_SAVE) {
        WorkspaceEntry entry; advanced_.CapturePreferences(entry); entry.mode = displayed_mode_;
        CValueDialog dialog(this,L"Save comparison preset",L"Name (1-80 characters). Saves comparison rules/view options only. File paths, drafts, anchors, baseline and synchronization actions are excluded.",L""); dialog.validate = [](const auto& value,std::wstring*) { return !value.empty() && value.size() <= 80 && value.find_first_of(L"\r\n\t") == std::wstring::npos; };
        path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer != IDOK) return;
        auto found = std::find_if(presets.begin(),presets.end(),[&](const auto& p) { return p.name == dialog.value; });
        if (found != presets.end()) { path_dialog_ = true; const auto replace = UiMessageBox(L"Replace the existing named preset?",MB_OKCANCEL | MB_ICONQUESTION); path_dialog_ = false; if (replace != IDOK) return; found->settings = PresetSettings(entry); }
        else presets.push_back({dialog.value,PresetSettings(entry)});
        status_.SetPaneText(0,SaveComparePresets(preset_path_,presets,&error) ? L"Named comparison settings saved beside the portable executable." : error.c_str()); return;
    }
    if (presets.empty()) { status_.SetPaneText(0,L"No named comparison settings saved."); return; }
    std::vector<std::vector<std::wstring>> rows; for (const auto& preset : presets) rows.push_back({preset.name,std::to_wstring(preset.settings.mode)});
    CReviewList dialog(this,command == ID_WORKFLOW_PRESET_DELETE ? L"Delete named settings" : L"Apply named settings - matching comparison mode only",{L"Name",L"Mode (2 Text, 3 merge, 4 folder, 5 CSV, 6 TSV, 7 JSON)"},std::move(rows)); dialog.accept_label = command == ID_WORKFLOW_PRESET_DELETE ? L"Delete selected" : L"Apply selected"; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer != IDOK || dialog.selected.size() != 1) return;
    const auto index = dialog.selected[0];
    if (command == ID_WORKFLOW_PRESET_DELETE) { presets.erase(presets.begin() + index); status_.SetPaneText(0,SaveComparePresets(preset_path_,presets,&error) ? L"Selected preset deleted." : error.c_str()); return; }
    if (presets[index].settings.mode != displayed_mode_) { status_.SetPaneText(0,L"Preset mode differs. Choose the matching comparison mode before applying."); return; }
    WorkspaceEntry entry; advanced_.CapturePreferences(entry); const auto saved = PresetSettings(presets[index].settings);
    // Preserve source-specific anchors and synchronization safety state.
    const auto anchors = entry.manual_anchors,baseline = entry.sync_baseline; const auto policy = entry.sync_policy; const bool deletions = entry.sync_deletions;
    entry = saved; entry.manual_anchors = anchors; entry.sync_baseline = baseline; entry.sync_policy = policy; entry.sync_deletions = deletions;
    advanced_.RestorePreferences(entry); advanced_.InvalidateInputs(); status_.SetPaneText(0,L"Named settings applied. Existing output is retained; press Compare to recompute. No synchronization started.");
}
