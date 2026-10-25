#include "advanced_compare.h"
#include "feature_dialogs.h"
#include "../resource.h"
#include "../core/manual_alignment.h"
using namespace simple_editor::core;
void CAdvancedCompareView::FeatureSettings(UINT command) {
    if (Busy()) return;
    if (command == ID_FEATURE_RENAMES) { StartContentPairing(); return; }
    if (command == ID_FEATURE_ANCHORS) { if (mode_ != 2) { Status(L"Manual line anchors require two-way Text/source mode."); return; } CFeatureInput dialog(this,L"Manual line alignment",L"One pair per line: left-line,right-line (starting at 1). Both sides must increase. Compare again to adopt anchors as a read-only view. Clear all pairs and compare again to restore merging. Existing output is retained until recompare is accepted.",manual_anchors_); dialog.validate = [](const std::wstring& text,std::wstring* error) { LineAnchors anchors; return ParseLineAnchors(text,&anchors,error); }; modal_ = true; const auto answer = dialog.DoModal(); modal_ = false; if (answer == IDOK) { manual_anchors_ = dialog.value; InvalidateInputs(); } return; }
    if (command == ID_FEATURE_TABLE_RULES) {
        if (mode_ != 5 && mode_ != 6) { Status(L"Choose CSV or TSV comparison first."); return; }
        CFeatureInput dialog(this,L"Table keys and columns",L"Optional first line: header=yes. Then left-column,right-column,key/text/tolerance, one mapping per line. Example: 1,2,key and 3,4,0.01. Empty settings retain positional comparison. Duplicate keys are reported as ambiguous.",table_rules_); dialog.validate = [](const std::wstring& text,std::wstring* error) { TableCompareOptions options; return ParseTableOptions(text,&options,error); }; modal_ = true; const auto answer = dialog.DoModal(); modal_ = false; if (answer == IDOK) { table_rules_ = dialog.value; InvalidateInputs(); } return;
    }
    if (command == ID_FEATURE_TEXT_RULES) {
        if (mode_ != 2) { Status(L"Advanced ignore rules apply to two-way Text/source comparison; three-way conflicts retain their existing safety rules."); return; }
        CFeatureInput dialog(this,L"Text comparison rules",L"One rule per line: ignore-blank=yes/no, mask-timestamps=yes/no, ignore-prefix=text, ignore-contains=text, mask=text. Text rules are literal and case sensitive. Timestamp masking covers YYYY-MM-DD HH:MM:SS or YYYY-MM-DDTHH:MM:SS. Originals and output are retained.",text_rules_); dialog.validate = [](const std::wstring& text,std::wstring* error) { TextCompareRules rules; return ParseTextRules(text,&rules,error); }; modal_ = true; const auto answer = dialog.DoModal(); modal_ = false; if (answer == IDOK) { text_rules_ = dialog.value; InvalidateInputs(); } return;
    }
}
