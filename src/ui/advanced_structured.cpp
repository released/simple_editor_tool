#include "advanced_compare.h"
using namespace simple_editor::core;
void CAdvancedCompareView::StartStructured(std::unique_ptr<BinaryDocument> left,std::unique_ptr<BinaryDocument> right) {
    if (Busy()) return; const auto format = static_cast<StructuredFormat>(mode_ - 5); const auto encoding = static_cast<TextEncoding>((std::max)(0,encoding_.GetCurSel())); cancelled_ = false; job_kind_ = JobKind::Structured; progress_ = 0; shown_progress_ = UINT_MAX; SetBusy(true);
    TableCompareOptions options; std::wstring error; if (!ParseTableOptions(table_rules_,&options,&error)) { SetBusy(false); Status(error); return; }
    job_ = std::async(std::launch::async,[this,format,encoding,options,left = std::move(left),right = std::move(right)]() {
        Result result; result.kind = JobKind::Structured; result.structured = std::make_shared<StructuredComparison>(); std::wstring first,second;
        const auto read = [&](BinaryDocument* document,std::wstring* text) { if (!document || document->Size() > 16 * 1024 * 1024) { result.error = L"Structured comparison accepts up to 16 MiB per input."; return false; } std::vector<std::uint8_t> bytes; TextCodec codec; if (!document->Read(0,static_cast<std::size_t>(document->Size()),&bytes) || !document->ValidateSource(true,&result.error) || !DecodeText(bytes,encoding,text,&codec,&result.error)) { if (result.error.empty()) result.error = L"Could not read/decode a structured input."; return false; } return true; };
        if (read(left.get(),&first) && read(right.get(),&second)) CompareStructured(first,second,format,cancelled_,result.structured.get(),&result.error,options); return result;
    });
}
