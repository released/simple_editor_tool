#include "advanced_compare.h"
#include "feature_dialogs.h"
#include "../core/text_source_stamp.h"
using namespace simple_editor::core;
void CAdvancedCompareView::StartStructured(std::shared_ptr<TextInputSnapshot> left,std::shared_ptr<TextInputSnapshot> right) {
    if (Busy()) return; const auto format = static_cast<StructuredFormat>(mode_ - 5); const auto encoding = static_cast<TextEncoding>((std::max)(0,encoding_.GetCurSel())); cancelled_ = false; job_kind_ = JobKind::Structured; progress_ = 0; shown_progress_ = UINT_MAX; SetBusy(true);
    TableCompareOptions options; JsonCompareOptions json; std::wstring error; if ((format != StructuredFormat::Json && !ParseTableOptions(table_rules_,&options,&error)) || (format == StructuredFormat::Json && !ParseJsonRules(json_rules_,&json,&error))) { SetBusy(false); Status(error); return; }
    const auto right_encoding = static_cast<TextEncoding>((std::max)(0,encoding_right_.GetCurSel()));
    LaunchJob([&] { return std::async(std::launch::async,[this,format,encoding,right_encoding,options,json,left = std::move(left),right = std::move(right)]() {
        Result result; result.kind = JobKind::Structured; result.structured = std::make_shared<StructuredComparison>(); std::wstring first,second;
        if (!left || !right || !left->Materialize(cancelled_,&result.error) || !right->Materialize(cancelled_,&result.error)) return result;
        const auto read = [&](BinaryDocument* document,TextEncoding requested,std::wstring* text,const wchar_t* side) { if (!document || document->Size() > 16 * 1024 * 1024) { result.error = L"Structured comparison accepts up to 16 MiB per input."; return false; } std::vector<std::uint8_t> bytes; TextCodec codec; if (!document->Read(0,static_cast<std::size_t>(document->Size()),&bytes) || !document->ValidateSource(true,&result.error) || !DecodeText(bytes,requested,text,&codec,&result.error)) { if (result.error.empty()) result.error = L"Could not read/decode a structured input."; return false; } if (!result.codec_details.empty()) result.codec_details += L" | "; result.codec_details += std::wstring(side) + L": " + DescribeTextCodec(codec,*text); return true; };
        if (read(left->document.get(),encoding,&first,L"Left") && read(right->document.get(),right_encoding,&second,L"Right")) {
            CompareStructured(first,second,format,cancelled_,result.structured.get(),&result.error,options,json);
            if (source_snapshot_ready) result.left_snapshot = left->document->Snapshot(&result.error);
            for (const auto& input : {left,right}) { FileStamp stamp; const auto& path = input->document->Path(); if (!InspectTextSource(path,&stamp,&result.error) || !input->document->ValidateSource(true,&result.error)) return result; result.source_stamps.emplace_back(path,stamp); }
        } return result;
    }); });
    if (job_.valid()) { ++source_generation_; if (!source_snapshot_ready) SetSourcesChanged(false); }
}
void CAdvancedCompareView::EditJsonRules() {
    if (Busy() || mode_ != 7) { Status(L"Choose JSON comparison first."); return; }
    CFeatureInput dialog(this,L"JSON path rules",L"RFC 6901 pointers: ignore /metadata/generatedAt or key /devices /id. Quote pointers containing spaces. Array IDs must be unique strings/numbers. Ignore paths below a keyed array are rejected; ignore the whole array or use rules outside it. Originals remain unchanged; compare again to apply.",json_rules_);
    dialog.validate = [](const auto& text,std::wstring* error) { JsonCompareOptions rules; return ParseJsonRules(text,&rules,error); };
    modal_ = true; const auto answer = dialog.DoModal(); modal_ = false;
    if (answer == IDOK) { json_rules_ = dialog.value; InvalidateInputs(); if (changed) changed(); }
}
