#include "advanced_compare.h"
#include "feature_dialogs.h"
using namespace simple_editor::core;
void CAdvancedCompareView::StartStructured(std::unique_ptr<BinaryDocument> left,std::unique_ptr<BinaryDocument> right) {
    if (Busy()) return; const auto format = static_cast<StructuredFormat>(mode_ - 5); const auto encoding = static_cast<TextEncoding>((std::max)(0,encoding_.GetCurSel())); cancelled_ = false; job_kind_ = JobKind::Structured; progress_ = 0; shown_progress_ = UINT_MAX; SetBusy(true);
    TableCompareOptions options; JsonCompareOptions json; std::wstring error; if ((format != StructuredFormat::Json && !ParseTableOptions(table_rules_,&options,&error)) || (format == StructuredFormat::Json && !ParseJsonRules(json_rules_,&json,&error))) { SetBusy(false); Status(error); return; }
    const auto right_encoding = static_cast<TextEncoding>((std::max)(0,encoding_right_.GetCurSel()));
    job_ = std::async(std::launch::async,[this,format,encoding,right_encoding,options,json,left = std::move(left),right = std::move(right)]() {
        Result result; result.kind = JobKind::Structured; result.structured = std::make_shared<StructuredComparison>(); std::wstring first,second;
        const auto read = [&](BinaryDocument* document,TextEncoding requested,std::wstring* text,const wchar_t* side) { if (!document || document->Size() > 16 * 1024 * 1024) { result.error = L"Structured comparison accepts up to 16 MiB per input."; return false; } std::vector<std::uint8_t> bytes; TextCodec codec; if (!document->Read(0,static_cast<std::size_t>(document->Size()),&bytes) || !document->ValidateSource(true,&result.error) || !DecodeText(bytes,requested,text,&codec,&result.error)) { if (result.error.empty()) result.error = L"Could not read/decode a structured input."; return false; } if (!result.codec_details.empty()) result.codec_details += L" | "; result.codec_details += std::wstring(side) + L": " + DescribeTextCodec(codec,*text); return true; };
        if (read(left.get(),encoding,&first,L"Left") && read(right.get(),right_encoding,&second,L"Right")) CompareStructured(first,second,format,cancelled_,result.structured.get(),&result.error,options,json); return result;
    });
}
void CAdvancedCompareView::EditJsonRules() {
    if (Busy() || mode_ != 7) { Status(L"Choose JSON comparison first."); return; }
    CFeatureInput dialog(this,L"JSON path rules",L"RFC 6901 pointers: ignore /metadata/generatedAt or key /devices /id. Quote pointers containing spaces. Array IDs must be unique strings/numbers. Ignore paths below a keyed array are rejected; ignore the whole array or use rules outside it. Originals remain unchanged; compare again to apply.",json_rules_);
    dialog.validate = [](const auto& text,std::wstring* error) { JsonCompareOptions rules; return ParseJsonRules(text,&rules,error); };
    modal_ = true; const auto answer = dialog.DoModal(); modal_ = false;
    if (answer == IDOK) { json_rules_ = dialog.value; InvalidateInputs(); if (changed) changed(); }
}
