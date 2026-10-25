#include <cstddef>
#include <cstring>
#include "ILexer.h"
#include "LexerModule.h"

using Lexilla::LexerModule;
extern const LexerModule lmCPP, lmPython, lmHTML, lmJSON, lmPowerShell,
    lmMarkdown, lmSQL, lmYAML, lmBash, lmBatch, lmDiff, lmCmake;

extern "C" Scintilla::ILexer5* EditorCreateLexer(const char* name) {
    const LexerModule* modules[] = {&lmCPP,&lmPython,&lmHTML,&lmJSON,&lmPowerShell,&lmMarkdown,&lmSQL,&lmYAML,&lmBash,&lmBatch,&lmDiff,&lmCmake};
    for (auto* module : modules) if (name && strcmp(name,module->languageName) == 0) return module->Create();
    return nullptr;
}
