# Automatic function sidebar

## Function filter extension (2026-11-02)

- Request: search the automatic sidebar to quickly filter function names.
- Implementation plan: add a regular native Edit/label above the existing list;
  retain each document's query in its owned sidebar. Filter the existing bounded
  snapshot with ordinal case-insensitive substring matching, using a separate
  visible-row index mapping so original-line navigation stays correct.
- Preserve parser debounce/cancellation/version guards, source buffers/native
  Undo and language switching. Retire native owner-data rows before replacing
  mappings. Label no matches and partial-result limitations explicitly.
- Verify Debug/Release --outline-sidebar (filter/navigation/keyboard/Unicode
  query/tab independence/edit/Undo/localization/layout/lifetime) and affected
  --localization; focused Release core catalog/preferences checks, canonical
  Release and formal application smoke. Retire owned fixtures/test binaries and
  compiler outputs, retain concise evidence.
- Functional acceptance PASS: seven focused groups (Release core localization /
  preferences; Debug/Release native outline/sidebar and localization; canonical
  Release process smoke). Final native filtered image inspected. No source/Undo,
  INI or parser semantics changed. The initial static-label assertion incorrectly
  applied toolbar-button padding; final checks use actual font extents/bounds.
- Cleanup PARTIAL:775 compiler files removed; protected-file hashes match. All
  synthetic payloads self-cleaned. Automatic review blocked test-output/recipe
  retirement with unspecified "blocked by policy"; no alternate bypass. Earlier
  residuals and complete-folder20MB overage remain unresolved. Compact ownership,
  exact hashes, results and final size: verification/function-filter-20261102.json.

## Original sidebar scope

Request (2026-10-27): show functions automatically at the left of the active
Text document, including C/C++ headers, and navigate directly without a modal
outline dialog. Each open document must retain independent results.

Entry behavior: the Review command launches a definitions-only C/C++ modal
dialog. Existing Text/clone content fills the page. No sidebar is present.

Plan before source changes:
1. Extend the bounded C/C++ scanner for declarations used by headers, retaining
   the existing definitions-only API default. Handle MFC message-map macros so
   the supplied hex_view.cpp example resolves to actual function lines.
2. Add a native sidebar component with its own cancellable background snapshot,
   debounce, generation/path/revision checks and a bounded virtual function list.
   Disable stale navigation immediately after edits; never mutate source text.
3. Own the sidebar and draggable divider per CMainFrame Text session. Show it
   automatically for C/C++ source/header or explicit C/C++ syntax. Hide it on
   other pages/documents. Keep clone layout, caret, Undo, zoom and Save intact.
   The old Review command refreshes the nonmodal panel.
4. Add focused parser and native UI coverage for headers, supplied-source MFC
   macros, tab switching, edits/Undo, stale jobs, navigation, layout, language and
   close during analysis. Select affected existing editor/clone/zoom workflows.
5. Update both guides/catalogs. Build Debug/Release affected tests and canonical
   Release, smoke the final product, then retire owned scratch/test EXEs with
   protected-file hashes and verified project-bound cleanup. Finish <=20 MB.

Confirmed scope: the user also requested other programming languages. Include
Python, JavaScript/TypeScript, Java/C#, PowerShell, Shell, CMake and SQL; Batch
shows subroutine labels. Each lexer/path selects its own bounded parser. Plain
text, JSON, Markdown, XML/HTML, YAML and diff documents have no automatic function
sidebar. Explicit supported syntax overrides remain effective.
This remains bounded heuristic navigation, not a compiler or full AST. Templates,
macro-generated functions and complex declarators can remain explicit limits.
No new dependencies, plugin mechanism, remote access or process memory work.

Status: implementation and affected verification pass in Debug/Release. Three
core and eight native UI modes per configuration, canonical Release, twelve
formal process assertions and seven CLI exit cases pass. Both final native images
were inspected. Final cleanup/preservation/20-MB checks pass: 850 compiler files,
eleven test/MSBuild outputs and 74 owned scratch files retired. Both preservation
checkpoints matched 1,475 protected files. artifacts is absent; complete project
is about 18.52 MB. Evidence: verification/outline-sidebar-20261027.json. The
requested sidebar and confirmed language scope are complete within documented
heuristic and verification limits; no Git publication was performed.
