#include "../src/core/structured_compare.h"
#include <iostream>
#include <filesystem>
using namespace simple_editor::core;
int RunStructuredTests(const std::filesystem::path&) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; }; std::wstring error; std::atomic_bool cancel{false}; StructuredComparison result;
    check(CompareStructured(L"a,b\r\n\"x,y\",\"say \"\"hi\"\"\"\r\n",L"a,b\n\"x,y\",\"say \"\"hi\"\"\"\n",StructuredFormat::Csv,cancel,&result,&error) && result.different == 0 && result.compared == 4,L"CSV compares parsed fields across quoting and row-ending styles");
    check(CompareStructured(L"\"a\r\nb\",x,",L"\"a\r\nb\",y",StructuredFormat::Csv,cancel,&result,&error) && result.different == 2 && result.rows[0].path == L"Row 1, column 2" && result.rows[1].state == L"Removed",L"multiline quoted cells and trailing empty fields retain logical row/column positions");
    check(CompareStructured(L"a\tb\n1\t2\n",L"a\tb\n1\t3\n",StructuredFormat::Tsv,cancel,&result,&error) && result.different == 1,L"TSV fields use Tab delimiters");
    check(!CompareStructured(L"\"unterminated",L"",StructuredFormat::Csv,cancel,&result,&error) && !CompareStructured(L"a\"b",L"",StructuredFormat::Csv,cancel,&result,&error),L"malformed quoted delimited records are rejected");
    check(CompareStructured(L"{\"b\":[1,2],\"a\":\"中😀\"}",L" { \"a\":\"\\u4E2D\\uD83D\\uDE00\",\"b\":[1.00,2e0] } ",StructuredFormat::Json,cancel,&result,&error) && result.different == 0,L"JSON normalization ignores object key order/formatting and compares exact decimal values");
    check(CompareStructured(L"9007199254740993",L"9007199254740992",StructuredFormat::Json,cancel,&result,&error) && result.different == 1,L"JSON numbers retain precision beyond binary double integers");
    check(CompareStructured(L"[-0,0.0100,100]",L"[0,1e-2,1e2]",StructuredFormat::Json,cancel,&result,&error) && result.different == 0,L"JSON decimal normalization handles signs, scales and exponents exactly");
    check(CompareStructured(L"[1,2]",L"[2,1]",StructuredFormat::Json,cancel,&result,&error) && result.different == 2,L"JSON arrays preserve ordering");
    check(CompareStructured(L"{\"a\":1,\"b\":null}",L"{\"a\":\"1\",\"c\":true}",StructuredFormat::Json,cancel,&result,&error) && result.different == 3 && result.rows[0].state == L"Type changed",L"JSON result paths distinguish type changes, missing keys and additions");
    check(CompareStructured(L"{\"\\u0000\":\"\\u0000\"}",L"{}",StructuredFormat::Json,cancel,&result,&error) && result.rows[0].path.find(L"\\u0000") != std::wstring::npos && result.rows[0].path.find(L'\0') == std::wstring::npos,L"escaped JSON control characters remain visible and safe in result paths");
    check(!CompareStructured(L"{\"a\":1,\"a\":2}",L"{}",StructuredFormat::Json,cancel,&result,&error) && !CompareStructured(L"\"\\uD800\"",L"null",StructuredFormat::Json,cancel,&result,&error),L"duplicate keys and invalid escaped surrogate sequences are rejected");
    check(!CompareStructured(L"01",L"1",StructuredFormat::Json,cancel,&result,&error) && !CompareStructured(L"[1,]",L"[]",StructuredFormat::Json,cancel,&result,&error) && !CompareStructured(L"1e1000001",L"1",StructuredFormat::Json,cancel,&result,&error),L"invalid JSON grammar and oversized exponents are bounded");
    std::wstring left,right; for (int i = 0; i < 20001; ++i) { left += L"a\n"; right += L"b\n"; } check(CompareStructured(left,right,StructuredFormat::Csv,cancel,&result,&error) && result.different == 20001 && result.rows.size() == 20000 && result.truncated,L"structured result previews remain bounded while full difference counts are retained");
    cancel = true; const auto count = result.different; check(!CompareStructured(L"{}",L"{}",StructuredFormat::Json,cancel,&result,&error) && result.different == count,L"cancelled structured comparisons do not replace previously valid results");
    return failures;
}
