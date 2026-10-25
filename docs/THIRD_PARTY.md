# Native source dependencies

The editor component is built from pinned upstream C++ sources and statically
linked with the same Debug/Release CRT as the host MFC application. No Python
runtime, Scintilla DLL or Lexilla DLL is required on the target computer.

| Component | Version | Source archive SHA-256 |
| --- | --- | --- |
| [Scintilla](https://www.scintilla.org/ScintillaDownload.html) | 5.6.7 | `2309181E59E7DA4659F890DBC128DB7451CCD094748770E7BE674B830494587C` |
| [Lexilla](https://www.scintilla.org/LexillaDownload.html) | 5.5.4 | `5E0A21356E4E2F19D6A38ECD1C8172D5A2B7B4542940568B023386080DB90FE0` |

Original notices: [Scintilla License](../vendor/scintilla/License.txt),
[Lexilla License](../vendor/lexilla/License.txt).

The repository includes the upstream C/C++ implementation and generated headers
needed for native builds; upstream Python generators/tests are not included or
required. `NativeEditor.vcxproj` uses the Windows implementation and selected
C/C++, Python-language, HTML/XML, JSON, PowerShell, Markdown, SQL, YAML, shell,
batch, diff and CMake lexers. `lexer_catalogue.cpp` selects their native factories.
Intermediate libraries and objects are written under `artifacts/obj`.
