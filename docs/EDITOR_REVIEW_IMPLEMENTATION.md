# Approved editor and comparison improvements

Scope approved 2026-10-26 following the read-only ten-feature review. This ledger
records implementation, not competitor parity. Preserve all pre-existing changes.

## Baseline and contracts

- Native MFC x64/static CRT/Scintilla/Lexilla only. No new runtime, driver,
  interpreter, browser framework, downloaded source or external library.
- Entry delivery: 3,439,616 bytes; SHA256
  1B8668629F83F79F56309983D8859E4D8C3EFA1A3B46D44BA0583FD13E9A64DD.
- Preserve draft ownership, encoding/EOL, native Undo, generation checks,
  snapshot/save locks, maximized/single-instance startup and Ctrl+wheel behavior.
- Current workspace policy permits necessary temporary compiler/test growth;
  no per-build temporary exception is needed. Bound it, clean completed batches,
  use project-owned TEMP/fixtures, retain concise evidence and finish <=20 MB.
  Never delete outside this project or infer ownership from an extension alone.
- No publication, new dependencies or reuse of third-party source is authorized.

## Implementation and acceptance

| ID | Approved first-version behavior | State |
| --- | --- | --- |
| V01 | Read-only full left/right Text comparison sources, selection/search, original line mapping and synchronized navigation; preserve list/output and current input limits | Verified |
| V02 | Explicit preview of all-open text replacement, per-file selection and before/after view; revalidate all chosen drafts before applying, one Undo each, no disk writes | Verified |
| V03 | Explicit current-text-draft versus fresh disk snapshot comparison after an external change; no automatic reload/overwrite, preserve original tab and version guards | Verified |
| V04 | Bounded Markdown heading outline and guarded jump, ignoring fenced-code headings; preserve draft/Undo | Verified |
| V05 | Direct multiline literal Find/Replace input using existing engine/scopes; retain single-line regex contract and history | Verified |
| V06 | Informational exact moved-block pairing within a text comparison, ambiguity skipped, paired jump; no change to merge decisions or reports' raw text | Verified |
| V07 | Several manual persistent Text mark colors, bounded native search and clear/edit invalidation; no character/Undo changes | Verified |
| V08 | Endian-aware named bitfields/flags inspection and explicit guarded editing; preserve other bits, one Undo and a documented bit numbering rule | Verified |
| V09 | Bounded EditorConfig indentation subset with root/section precedence and explicit unsupported diagnostics; no automatic charset/EOL rewriting or full-conformance claim | Verified |
| V10 | Native IPS/IPS32 import/export for equal-length changes; explicit base SHA verification, offset/size guards, reviewed import on a private snapshot and one Undo | Verified |

Batch A: V01/V02/V03. Batch B: V04/V05/V07/V09. Batch C: V06/V08/V10.
Implement reusable core models, native controls/commands, then focused core and
Debug/Release UI checks; do not change shared layouts during compilation.

## Verification and delivery

See docs/verification/editor-review-20261026.json for actual test, binary,
source, INI and final cleanup records. Product usage is in docs/EDITOR_REVIEW.md.

- Debug and Release: core --editor-review, --search-codec, --preferences,
  --presentation and --text-reports PASS. New core review mode records 38 checks.
- Debug and Release: UI --editor-review, --text-workflow, --pasted-text,
  --advanced, --workspace, --zoom and --preferences PASS. New UI review mode
  records 59 checks, including two native image captures. Source-search fixtures
  explicitly activate the window before testing focus; routing also survives
  focus moving into the modeless dialog. Read-only source Undo preserves output.
- Canonical portable Release and --workspace-process PASS; seven CLI cases return
  0/0/1/1/2/1/2. Native x64, ASLR/high-entropy/NX and Windows-only imports PASS.
- Product: build/SimpleEditorTool.exe, 3,573,248 bytes, SHA-256
  0F8D74D3511EBA7DE36092F90FA39F1C1E65885EFDCE91E0C16A25B07AFD1BCF.
- Full quota stress/throughput, physical input/mixed DPI, allocation/power failure
  and broad third-party patch interoperability were not run. EditorConfig is an
  explicit indentation subset; patches are equal-length; moved blocks are exact
  unique unmatched runs. No feature/runtime/dependency pruning was used.

## Changed components

- src/core/text_review.*, editor_config.*, bit_fields.* and ips_patch.* implement
  bounded parsing, discovery and edit plans without new dependencies.
- src/ui/advanced_sources.cpp and advanced_compare* connect full sources, maps,
  moved navigation and source Undo guards. code_editor* owns native mark colors.
- src/ui/review_dialogs.*, search_multiline.cpp, editor_dialogs* and
  feature_dialogs* provide tree, multiline and selected-file preview controls.
- src/main_frame_review.cpp, main_frame_text_review.cpp, main_frame_ips.cpp and
  main_frame_binary_review.cpp connect asynchronous review jobs and safe edits;
  existing frame, clone, search and workspace routing preserve source/draft state.
- NativeCoreTests/NativeUiTests/product project entries, resources and the two
  native_editor_review test sources register and exercise the native flows.
- README/SPEC/agent/handoff/self-check and docs/EDITOR_REVIEW.md document available
  behavior and separate product instructions from engineering acceptance.

EditorConfig operation follow-up: Enter after an opening brace uses the effective
indentation size (2) independently of visual tab width (8). Remembering a file's
view does not leak its EditorConfig/manual indentation into the global defaults.
Debug native --editor-review and --code-editor/--preferences pass; final Release
follow-up and final cleanup results are recorded in the acceptance JSON.
