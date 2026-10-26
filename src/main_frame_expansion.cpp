#include "main_frame.h"
#include "resource.h"
#include "ui/feature_dialogs.h"
void CMainFrame::AppendExpansionCommands(CMenu& review) {
    review.AppendMenu(MF_SEPARATOR);
    review.AppendMenu(MF_STRING,ID_WORKFLOW_BASE_SOURCE,L"Show / hide full Base source");
    review.AppendMenu(MF_STRING,ID_WORKFLOW_TEXT_TO_HEX,L"Show Text selection in Hex");
    review.AppendMenu(MF_STRING,ID_WORKFLOW_HEX_TO_TEXT,L"Show Hex selection in Text");
    CMenu presets; presets.CreatePopupMenu(); presets.AppendMenu(MF_STRING,ID_WORKFLOW_PRESET_SAVE,L"Save current comparison settings..."); presets.AppendMenu(MF_STRING,ID_WORKFLOW_PRESET_APPLY,L"Apply named settings..."); presets.AppendMenu(MF_STRING,ID_WORKFLOW_PRESET_DELETE,L"Delete named settings..."); review.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(presets.Detach()),L"Comparison presets");
    CMenu unicode; unicode.CreatePopupMenu(); unicode.AppendMenu(MF_STRING,ID_WORKFLOW_UNICODE_SCAN,L"Inspect unusual Unicode..."); unicode.AppendMenu(MF_STRING,ID_WORKFLOW_UNICODE_LIVE,L"Toggle live Unicode notices"); unicode.AppendMenu(MF_STRING,ID_WORKFLOW_UNICODE_ALLOW,L"Allowed Unicode code points..."); review.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(unicode.Detach()),L"Unicode notices");
    review.AppendMenu(MF_STRING,ID_WORKFLOW_STATISTICS,L"Interactive binary statistics...");
    review.AppendMenu(MF_STRING,ID_WORKFLOW_CONVERSION,L"Base64 / Hex / UTF-8 conversion...");
    review.AppendMenu(MF_STRING,ID_WORKFLOW_JSON_RULES,L"JSON ignored paths / array keys...");
}
void CMainFrame::OnExpansion(UINT command) {
    if (command == ID_WORKFLOW_BASE_SOURCE) { if (AdvancedActive()) advanced_.ToggleBaseSource(); return; }
    if (IsBusy()) return;
    if (command == ID_WORKFLOW_TEXT_TO_HEX || command == ID_WORKFLOW_HEX_TO_TEXT) { if (!StartSelectionMap(command == ID_WORKFLOW_TEXT_TO_HEX)) status_.SetPaneText(0,L"Choose the corresponding file Text / Hex page first."); return; }
    if (command >= ID_WORKFLOW_PRESET_SAVE && command <= ID_WORKFLOW_PRESET_DELETE) { PresetCommand(command); return; }
    if (command == ID_WORKFLOW_JSON_RULES) { if (AdvancedActive()) advanced_.EditJsonRules(); return; }
    if (command == ID_WORKFLOW_SOURCE_KEEP) { source_dismissed_key_ = source_notice_key_; source_notice_visible_ = false; Layout(); return; }
    if (command == ID_WORKFLOW_SOURCE_COMPARE) { StartDiskReview(); return; }
    if (command == ID_WORKFLOW_SOURCE_RELOAD) { OnReload(); return; }
    if (command == ID_WORKFLOW_UNICODE_SCAN) { StartUnicodeScan(true); return; }
    if (command == ID_WORKFLOW_UNICODE_LIVE) { unicode_live_ = !unicode_live_; unicode_generation_ = UINT64_MAX; if (!unicode_live_) { text_.ClearUnicodeNotices(); if (text_clone_.GetSafeHwnd()) text_clone_.ClearUnicodeNotices(); } session_menu_.CheckMenuItem(command,MF_BYCOMMAND | (unicode_live_ ? MF_CHECKED : MF_UNCHECKED)); status_.SetPaneText(0,unicode_live_ ? L"Live Unicode notices enabled for this file Text buffer." : L"Live Unicode notices disabled."); return; }
    if (command == ID_WORKFLOW_UNICODE_ALLOW) { CValueDialog dialog(this,L"Allowed Unicode points",L"Comma/space-separated hexadecimal scalars, e.g. U+00A0 U+200B. Applies to notices only; text stays unchanged.",unicode_allowlist_); std::set<std::uint32_t> allowed; dialog.validate = [&](const auto& value,std::wstring* error) { return simple_editor::core::ParseUnicodeAllowlist(value,&allowed,error); }; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer == IDOK) { unicode_allowlist_ = dialog.value; unicode_allowed_ = std::move(allowed); unicode_generation_ = UINT64_MAX; text_.ClearUnicodeNotices(); if (text_clone_.GetSafeHwnd()) text_clone_.ClearUnicodeNotices(); } return; }
    if (command == ID_WORKFLOW_STATISTICS) { StartStatistics(); return; }
    if (command == ID_WORKFLOW_CONVERSION) { ConversionDialog(); return; }
    status_.SetPaneText(0,L"Choose the relevant Text, Hex or comparison page.");
}
