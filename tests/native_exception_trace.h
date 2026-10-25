#pragma once
#include <DbgHelp.h>
#include <cstdio>
#pragma comment(lib,"Dbghelp.lib")
inline LONG CALLBACK NativeExceptionTrace(EXCEPTION_POINTERS* exception) {
    const auto code = exception->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != 0xC000041D && code != EXCEPTION_STACK_OVERFLOW) return EXCEPTION_CONTINUE_SEARCH;
    static LONG tracing = 0; if (InterlockedCompareExchange(&tracing,1,0)) return EXCEPTION_CONTINUE_SEARCH;
    char text[1024]{}; DWORD written = 0;
    sprintf_s(text,"NATIVE EXCEPTION 0x%08lX at %p\n",code,exception->ExceptionRecord->ExceptionAddress); WriteFile(GetStdHandle(STD_ERROR_HANDLE),text,static_cast<DWORD>(strlen(text)),&written,nullptr);
    if (code == EXCEPTION_STACK_OVERFLOW) return EXCEPTION_CONTINUE_SEARCH;
    void* frames[40]{}; const auto count = CaptureStackBackTrace(0,40,frames,nullptr);
    for (unsigned i = 0; i < count; ++i) {
        char storage[sizeof(SYMBOL_INFO) + 512]{}; auto* symbol = reinterpret_cast<SYMBOL_INFO*>(storage); symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = 511; DWORD64 displacement = 0;
        const bool found = SymFromAddr(GetCurrentProcess(),reinterpret_cast<DWORD64>(frames[i]),&displacement,symbol) != FALSE;
        sprintf_s(text,"  %p %s + 0x%llX\n",frames[i],found ? symbol->Name : "<system>",displacement); WriteFile(GetStdHandle(STD_ERROR_HANDLE),text,static_cast<DWORD>(strlen(text)),&written,nullptr);
    }
    InterlockedExchange(&tracing,0); return EXCEPTION_CONTINUE_SEARCH;
}
inline void InitializeNativeExceptionTrace() { SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS); SymInitialize(GetCurrentProcess(),nullptr,TRUE); AddVectoredExceptionHandler(1,NativeExceptionTrace); }
