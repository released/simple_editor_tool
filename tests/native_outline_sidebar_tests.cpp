#include "../src/core/function_outline.h"
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
int RunOutlineSidebarTests(const std::filesystem::path&) {
    int failures = 0; std::atomic_bool cancel{false}; bool truncated = false; std::vector<MarkdownHeading> rows;
    const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
    const auto parse = [&](const std::wstring& text,OutlineLanguage language) { rows.clear(); truncated = false; return ProgramFunctionOutline(text,language,cancel,&rows,&truncated); };
    const auto has = [&](const wchar_t* text) { return std::any_of(rows.begin(),rows.end(),[&](const auto& row) { return row.title.find(text) != std::wstring::npos; }); };
    for (const auto name : {L"sample.h",L"sample.HPP",L"sample.hxx",L"sample.hh",L"sample.inl",L"sample.cc",L"sample.c++"}) check(CppOutlinePath(name),L"C/C++ source/header extension participates in automatic outline");
    check(OutlineLanguageFor(L"data.json","json") == OutlineLanguage::None && OutlineLanguageFor(L"a.md","markdown") == OutlineLanguage::None && OutlineLanguageFor(L"a.txt","cpp",true) == OutlineLanguage::Cpp,L"data documents hide the sidebar and explicit syntax overrides are honored");
    check(OutlineLanguageFor(L"a.ts","cpp") == OutlineLanguage::JavaScript && OutlineLanguageFor(L"a.cs","cpp") == OutlineLanguage::Managed && OutlineLanguageFor(L"a.java","cpp") == OutlineLanguage::Managed,L"shared C++ lexer styles do not select the wrong language parser");
    const std::wstring header = L"#pragma once\nextern \"C\" int api(int x);\nclass Worker {\npublic:\n Worker();\n ~Worker();\n virtual void run() = 0;\n Worker(const Worker&) = default;\n int read(int x) const;\n int inline_fn() { return 1; }\n void (*callback)(int);\n int field = unrelated();\n DECLARE_MESSAGE_MAP()\n};\nusing Fn = int (*)(int);\n";
    check(parse(header,OutlineLanguage::Cpp) && rows.size() == 7 && has(L"api") && has(L"~Worker") && has(L"run") && has(L"inline_fn") && !has(L"callback") && !has(L"unrelated") && !has(L"DECLARE"),L"header prototypes, constructors, pure/default members and inline bodies exclude fields and macros");
    check(!rows.empty() && rows.front().line == 2 && rows.front().title.back() == L';',L"header declaration points to its exact original source line");
    const std::wstring mapped = L"BEGIN_MESSAGE_MAP(C, Base)\n ON_WM_CREATE()\n ON_MESSAGE(MSG, handler)\nEND_MESSAGE_MAP()\nint C::OnCreate(int x) { return x; }\n";
    check(parse(mapped,OutlineLanguage::Cpp) && rows.size() == 1 && rows[0].line == 5 && has(L"OnCreate") && !has(L"BEGIN_MESSAGE_MAP"),L"MFC message-map calls do not become functions or shift the next definition");
    std::ifstream file(std::filesystem::current_path() / L"src" / L"hex_view.cpp",std::ios::binary); const std::string source{std::istreambuf_iterator<char>(file),{}}; std::wstring supplied(source.begin(),source.end());
    check(!source.empty() && parse(supplied,OutlineLanguage::Cpp) && has(L"OnCreate") && has(L"SetZoomPercent") && !has(L"BEGIN_MESSAGE_MAP"),L"user-supplied hex_view.cpp example lists actual functions");
    check(parse(L"# def fake():\ntext = '''\ndef hidden(): pass\n'''\nclass C:\n def method(self):\n  def nested(): pass\nasync def work(\n x):\n pass\n",OutlineLanguage::Python) && rows.size() == 3 && rows[0].line == 6 && has(L"nested") && has(L"work") && !has(L"hidden"),L"Python async/method/nested definitions ignore comments and multiline strings");
    check(parse(L"// function fake() {}\nconst regex = /function hidden() {}/;\nfunction named(x) { return x; }\nconst arrow = async (x) => x;\nconst single = x => x;\nconst typed = (x: number): number => x;\nclass C {\n method(x: number): number { return x; }\n declaration(x: number): void;\n}\n",OutlineLanguage::JavaScript) && rows.size() == 6 && has(L"typed") && has(L"named") && has(L"arrow") && has(L"single") && has(L"method") && has(L"declaration") && !has(L"hidden"),L"JavaScript/TypeScript names, arrows, class methods and signatures exclude comments/regular expressions");
    check(parse(L"class C { public C() {} public int Run(int x) { return x; } public void Stop(); }",OutlineLanguage::Managed) && has(L"Run") && has(L"Stop"),L"Java/C# method and interface-style declaration subset is available");
    check(parse(L"# function Fake {}\n@'\nfunction Hidden {}\n'@\nfunction Get-Thing { 1 }\nfilter Select-Thing { $_ }\n",OutlineLanguage::PowerShell) && rows.size() == 2 && has(L"Get-Thing") && !has(L"Hidden"),L"PowerShell function/filter blocks ignore comments and here strings");
    check(parse(L"# fake() {}\ncat <<EOF\nhidden() {}\nEOF\nreal() { :; }\nfunction another { :; }\n",OutlineLanguage::Shell) && rows.size() == 2 && has(L"real") && has(L"another") && !has(L"hidden"),L"Shell function forms ignore comments and simple here documents");
    check(parse(L"#[=[\nfunction(hidden)\n]=]\nfunction(First arg)\nendfunction()\nMACRO(Second)\nendmacro()\n",OutlineLanguage::CMake) && rows.size() == 2 && has(L"First") && has(L"Second"),L"CMake function/macro declarations ignore bracket comments");
    check(parse(L"-- CREATE FUNCTION hidden()\nSELECT 'CREATE PROCEDURE fake';\nCREATE OR REPLACE FUNCTION first() RETURNS INT;\nCREATE PROCEDURE second AS SELECT 1;",OutlineLanguage::Sql) && rows.size() == 2 && has(L"first") && has(L"second"),L"SQL routine declarations ignore comments and string values");
    check(parse(L"@echo off\n:: comment\n:worker\necho x\n:eof\n:next_task\n",OutlineLanguage::Batch) && rows.size() == 2 && has(L"worker") && has(L"next_task"),L"Batch subroutine labels exclude comments and EOF marker");
    const auto previous = rows; cancel = true; check(!ProgramFunctionOutline(L"def a(): pass",OutlineLanguage::Python,cancel,&rows,&truncated) && rows.size() == previous.size(),L"cancelled analysis retains its prior result vector"); cancel = false;
    std::wstring many; for (unsigned n = 0; n < 4100; ++n) many += L"def f" + std::to_wstring(n) + L"(): pass\n"; check(parse(many,OutlineLanguage::Python) && truncated && rows.size() == 4096,L"automatic outline retains at most 4096 functions");
    return failures;
}
