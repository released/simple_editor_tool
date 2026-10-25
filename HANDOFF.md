# Handoff

## Explorer menu and Hex scroll boundaries (2026-10-26)

- Complete: current-user file context menu is enabled as "以 Simple Editor Tool 開啟",
  targeting the current Release EXE. Manage it from Tools > Windows Explorer >
  Add file context menu / Remove file context menu. Works before and after files
  open. This is a static classic-menu verb; Windows 11 may require Show more options.
- Changed application files: services/explorer_menu.h/.cpp own bounded per-user
  registry queries, quoted EXE/input paths, ownership checks, rollback and Shell
  refresh; workspace_shell.cpp owns native menu commands/feedback/update state.
  workspace_frame.h/.cpp and main_frame.cpp attach menus and route commands;
  resource.h defines the two IDs. Product/UI projects register the new sources.
  No installer, admin requirement, shell DLL, driver, default association or HKLM write.
- Hex fix: src/hex_view.cpp returns before scrollbar update/invalidation/sync
  notification when OnVScroll leaves the viewport unchanged. Real movement uses
  Invalidate(FALSE), retaining drawing while avoiding background erase. Resize,
  zoom and content-change invalidation remain separate. This prevents repeated
  bottom/top wheel events from repainting either synchronized comparison pane.
- Tests changed: native_explorer_ui.inc, native_ui_tests.cpp and
  native_workspace_process_tests.cpp. Final Debug/Release each PASS: isolated
  Explorer registry/menu 13, workspace 35, zoom/scroll 51, async-save 32 and
  exact byte-matched formal Shell process 12. The latter actually queries the
  Windows IContextMenu and invokes the registered Shell verb on Unicode/spaced/
  ampersand/percent paths, retains multiple tabs and tests Add/Remove/normal exit.
  Thirty repeated wheel events at each Hex boundary request no repaint/callback;
  normal movement, Ctrl+wheel, saved state and font/Undo checks pass.
- Canonical scripts/build.ps1 -Configuration Debug and scripts/build_release.bat
  PASS; native UI test builds PASS. No reported compiler/linker errors/warnings.
  No exact target instance required closure in final canonical builds.
- Current Release build/SimpleEditorTool.exe: 3,394,560 bytes; SHA-256
  63889239C8C511840C4541A91946C7ED3A479C183EEEA671AB1D0A6113BE45A2.
  x64/ASLR/high-entropy/NX and Windows-only imports verified. Static MFC/CRT/editor.
  build contains only EXE plus three existing INIs. Registry activation verifies
  owner/icon/model and exact quoted command in HKCU and the merged Shell view.
- The active settings INI changed from 833 to 901 bytes at 00:20 while work was
  ongoing; its origin was not attributed, and no fixture temp paths were found
  in its recent entries. Latest settings were retained. The two other INIs match
  turn entry; all three latest hashes match the final checkpoint. Do not restore
  historical user settings. Current numeric evidence: docs/verification/explorer-20261026.json.
- Verification commands: MSBuild NativeUiTests.vcxproj for Debug/Release x64;
  run --explorer and --workspace per configuration, then --zoom, --async-save and
  --explorer-process <formal-exe>. Final activation used the same tested registry
  service, followed by readback. Logs: artifacts/logs/explorer-*; source manifest:
  artifacts/evidence/explorer-final-inputs.json (228 inputs; 9 changed, 4 added).
- Fixture corrections: use public MFC command-update routing instead of opaque
  handler internals; dismiss either standard informational-dialog button in the
  test-owned process; pending client update regions are asserted only for visible
  movement fixtures. No product workaround for those fixture assumptions.
- Final review: no remaining high/medium finding. Physical Explorer/wheel/DPI,
  clean-PC installation, 100-file simultaneous selection and injected registry
  mid-write failure NOT RUN. Unchanged core/CLI/clean evidence is reused from
  earlier scopes; no unrelated full suite or clean audit rerun. No publication.
- Earlier records below are historical and do not identify the current EXE.

## Ctrl+wheel zoom correction (2026-10-25)

- Request/result: ordinary wheel scrolls; Ctrl+wheel zooms. Keep per-view percentage,
  50%-300% bounds, Ctrl+0, draft/selection/Undo and save-state guards.
- Application changes: src/ui/wheel_zoom_control.h requires MK_CONTROL before
  invoking zoom; src/main_frame.cpp applies the same condition to hovered-content
  pretranslation; src/ui/text_pager.cpp requires Ctrl in the large-text viewer.
  This restores native scrolling in Hex, Text/shared views, binary/advanced result
  lists and merge output. No new interface, dependency or build flag.
- tests/native_ui_tests.cpp now sends explicit Ctrl for zoom and verifies real
  scroll positions with ordinary wheel, unchanged percentages, shared-view routing,
  advanced result/output scrolling and pager scroll/zoom. Save-time fixture uses
  Ctrl+wheel explicitly. README/SPEC/AGENTS/docs/LARGE_FILES.md describe the rule.
- Canonical Debug/Release builds PASS. Native UI Debug and Release each PASS:
  --zoom 47 assertions; --async-save 32 assertions; exact byte-matched formal
  --workspace-process <exe> 12 assertions. No reported compiler/linker warnings
  or errors. No matching target process required pre-build closure.
- Release build/SimpleEditorTool.exe: 3,384,320 bytes; SHA-256
  E445063B941EB1987850889525B7C953F618AE294BEC3C0C5D8876E6308BF13F.
  build contains exactly EXE and three existing INIs. The user's settings INI
  already had a 23:23 update before the 23:27 build; latest settings are retained.
  Its hash differs from the older N01-N09 record. A turn-entry INI hash snapshot
  was not retained; current hashes/timestamps and this verification gap are in
  docs/verification/ctrl-wheel-20261025.json. No old settings were restored.
- One initial synthetic Ctrl+0 assertion failed and PowerShell Stop aborted that
  console attempt on stderr. No Ctrl+0 code changed; two complete Debug reruns
  and the Release run passed all 47 zoom cases. Raw note/logs: artifacts/logs/ctrl-wheel-*.
- Verification commands: build NativeUiTests.vcxproj with MSBuild for Debug/Release
  x64; run each above filter separately; scripts/build.ps1 -Configuration Debug
  and scripts/build_release.bat create the actual product EXEs. Source manifest:
  artifacts/evidence/ctrl-wheel-final-inputs.json (224 inputs, only four source/test
  files changed from the previous accepted source manifest).
- Review: no remaining high/medium finding in changed behavior. Physical wheel/key
  input and multi-monitor DPI are NOT RUN; native messages/state fixtures were used.
  Unchanged core/CLI/PE/clean-script evidence is reused from the previous record;
  no unrelated suite or clean audit rerun. No publication action.
- Earlier records below describe historical versions, not the current EXE identity.

## Completed native workflows N01-N09 (2026-10-25)

- All nine approved native first versions are complete. Active ledger:
  docs/NATIVE_WORKFLOW_IMPLEMENTATION.md. User guide: docs/NATIVE_WORKFLOWS.md.
  Changed files, checks and limits: docs/verification/NATIVE_WORKFLOW_ACCEPTANCE.md;
  exact identity/counts: docs/verification/native-workflows-20261025.json.
- Canonical Debug and Release passed. Release build/SimpleEditorTool.exe:
  3,384,320 bytes; SHA-256
  B4D36D863CF3F9165F773D9B0CC8DE561770BF4B19ED8017300458C4B96B10EF.
  Static native MFC/CRT/editor; Windows system imports only; no new driver/runtime.
  build contains exactly EXE + original three INIs with unchanged original hashes.
- Core PASS: workflows 37, merge-sync 47, folder-tools 31, file-safety 28,
  recovery 11, discovery 20. Native Debug/Release each PASS: workflows 43
  (including six captures), code-editor 26, search-tools 32, discovery 19,
  discovery-completion 3, workspace 35, advanced 52, async-save 32, zoom 34,
  recovery 13, folder-tools 12. Final column/deep-tree changes reran core workflows
  and native workflows/advanced; unchanged results from this scope are reused.
- Exact formal Debug/Release copies each PASS: 12 GUI process assertions and
  7 CLI cases. Six actual native workflow captures per configuration inspected.
  PE: x64/ASLR/high-entropy/NX. Final source/project manifest: 224 inputs under
  artifacts/evidence/native-workflows-final-inputs.json; logs: artifacts/logs/workflow-*.
- Actual clean audit PASS: static 99 scripts; fixture 17 protected files/source,
  idempotence and junction rejection; actual 18 protected files SHA unchanged,
  699 allowlisted compiler intermediates removed. The next build regenerates obj.
- Commands: scripts/build_release.bat; scripts/build.ps1 -Configuration Debug;
  NativeCoreTests.exe --workflows plus the affected filters listed above;
  NativeUiTests.exe --workflows <capture-dir> plus listed filters in both configs;
  NativeUiTests.exe --workspace-process <formal-exe>; scripts/verify_cli.ps1;
  shared verify_clean_scripts.ps1 and verify_clean_preservation.ps1 with
  -IntermediateRoot artifacts\obj -CheckNestedDelivery -CheckReparseRejection
  -CleanActualProject. No matching target instance required pre-build closure.
- No active build/test session or outstanding approved implementation item.
  Final review: no remaining high/medium finding. Bounds and NOT RUN physical,
  DPI/network/elevated/clean-PC coverage are explicit in acceptance. Earlier
  completed records below are historical, not the current EXE identity.
- Publication remains deferred; no GitHub remote, commit or upload was created.
## Completed feature-discovery delivery (2026-10-25)

- All twelve approved native first versions R01-R12 are complete. Source changes,
  acceptance commands/results and limitations: docs/verification/FEATURE_DISCOVERY_ACCEPTANCE.md;
  exact identity/counts: docs/verification/discovery-20261025.json. The active ledger
  docs/FEATURE_DISCOVERY_IMPLEMENTATION.md has twelve Complete/PASS rows.
- Release build/SimpleEditorTool.exe: 3,271,680 bytes, SHA-256
  41C83EACD05C2B0FD7097212B14A98A8136D9729447B92D76CCAE0A1EE7AABCA.
  Canonical Debug and Release passed. Static native MFC/CRT/editor; no added library,
  Python/.NET/Qt/runtime/driver installation requirement. Older artifact records
  below are historical and do not replace this final identity.
- Core PASS: discovery 20, discovery-analysis 25, affected text-reports 22 and
  merge-sync 47. Native Debug/Release each PASS: discovery 19, discovery-flows 19,
  visual with captures 17, completion 3, advanced 52, workspace 35, search-tools 32,
  recovery 13, async-save 32, zoom 34, text-reports 13. Reuse unaffected evidence
  after the completion-only correction; no unrelated full suite rerun.
- Batch tests actually covered preview/no writes, skipped invalid input, UTF-8
  BOM/CRLF and UTF-16 BOM/LF, metadata-restored content changes, dirty-editor
  refusal, post-first-commit cancellation and journal recovery. Completion after
  shared-view Save As reads the owner's current language and rejects stale typing.
- Actual byte-matched formal Debug/Release each passed 12 GUI process checks and
  7 CLI cases. x64/ASLR/high-entropy/NX and Windows-only imports checked. Six real
  native feature captures per configuration were inspected. Canonical builds have
  no reported compiler/linker warnings/errors.
- Clean static scan: 99 scripts PASS. Actual preservation: 17 fixture files,
  junction refusal/idempotence, 663 allowlisted intermediates removed and 18
  protected actual files SHA unchanged. build contains EXE + original three INIs
  only, original INI hashes unchanged. The next build regenerates cleaned objects.
- README stays product-facing; docs/EDITOR_AND_COMPARE_WORKFLOWS.md explains
  commands and bounds. Manual anchors are read-only two-way; advanced literal
  rules are two-way; batch writes are per-file atomic and can retain staging;
  tail follows bounded saved-disk UTF-8/UTF-16 windows. Completion dictionaries
  have Python/JSON sets and a common C-family fallback, without semantic analysis.
- NOT RUN: physical clipboard/drag/wheel/multiple monitors, native per-monitor DPI
  transitions, real network servers and clean-PC installation. No full competitor
  parity claim. No GitHub remote, commit or upload; publication remains deferred.
- No remaining required implementation/delivery work or active build/test command.
  Preserve all current source and original INIs; never reset untracked work.

## Completed alignment scope and delivery (2026-10-25)

- All 30 original recommendation rows now have implementation/evaluation
  dispositions and focused acceptance in docs/ALIGNMENT_IMPLEMENTATION.md.
  Read docs/verification/ALIGNMENT_ACCEPTANCE.md for changed components, evidence,
  reproduction commands and limits; exact data is in alignment-20261025.json.
  This does not claim complete competitor parity. X04 was evaluation, not a
  promised macro recorder; independent extension decisions are explicit in
  docs/EXTENSION_ASSESSMENT.md.
- Final canonical Release|x64: build/SimpleEditorTool.exe, 3,035,136 bytes,
  SHA-256 D216B7CE90D07909DEACE93F049ACF914FCA76FA00E08C11C189CFC82385A179.
  Debug canonical output: artifacts/debug/SimpleEditorTool.exe. Static native
  MFC/CRT/Scintilla/Lexilla; no Python, runtime or driver installation added.
- Implementations include native editor/search/Hex/report/merge/format/folder
  workflows, untitled/recent/tab actions, portable preferences, opt-in separate
  draft recovery, CLI, structured CSV/TSV/JSON comparison, bounded binary field
  templates/tree, large text paging and incremental binary comparison results.
  Original INIs and independent drafts/Undo/snapshot/save guards were preserved.
- Final core: CLI 14, structured 14, templates 10, paging 11; affected binary-tools
  12/text-reports 22/merge-tools 19 rerun. Debug/Release structured 6 and progressive
  compare 4 passed; latest inspection Debug 11 / Release 13 with actual captures.
  Earlier focused core/UI evidence remains in the ledger/SELF_CHECK.
- Exact formal Debug and Release EXE copies each passed 12 GUI process checks and
  seven CLI cases. Ready-window maximization, concurrent/renamed singleton,
  retained multi-file forwarding, selected close and normal release/relaunch
  passed. EXE imports, x64/ASLR/high-entropy/NX and delivery inventory passed.
- Quiet sparse 1 GiB core runs: page pairs 2/4/2 ms, full comparisons 650/638/656
  ms, cancellation 11/9/9 ms, peak working sets about 7.82 MB. These are core /
  synthetic measures only; sparse NUL holes were not decoded as valid text.
- Workspace static clean scan: 93 scripts passed. Actual preservation audit passed
  (17 fixture files; junction refusal/idempotence; 569 allowlisted objects removed;
  18 actual protected files unchanged). build contains exactly EXE + original
  three INIs. The next build will regenerate cleaned compiler/static-library files.
- No remaining implementation/delivery step in the agreed original scope.
  Physical clipboard/drag/wheel/multiple monitors and real server/device coverage
  are NOT RUN. Native per-monitor DPI transitions, archive/image/remote providers,
  disassembly/process/raw-device sources and macro/DLL plugin compatibility are
  not implemented; do not silently advertise them. Windows scaling remains.
- README remains product-facing; new guides cover CLI/structured/templates/paging.
  Generated logs/screenshots/dependency downloads are ignored. No GitHub remote,
  commit or upload was created: the user deferred publication.
- No build/job process from this turn remains active. Do not overwrite unrelated
  work or original INIs, and never mutate shared layouts during a dependent build.

## Feature alignment review (2026-10-25)

- User requested another feature-gap audit against WinMerge, Beyond Compare,
  Notepad++, MadEdit and Hex viewers. See `docs/FEATURE_ALIGNMENT_REVIEW.md`.
- Static source inspection plus primary documentation; the report separates
  existing/partial/missing behavior, priorities and proposed acceptance checks.
- Document checks passed: 30 feature rows, source references, local evidence
  links and table structure. Hash checks preserved all 70 baseline files outside
  this handoff, including README, application sources, delivered EXE and INIs.
- Preserve the product-only README. No app/source/EXE/INI changes, implementation,
  build, new runtime tests or GitHub publication were performed for this review.
- Exact Hex viewer remains unspecified; HxD/ImHex are explicit references.
  MadEdit fork features are labelled as family references. Next recommended work:
  Hex selection/clipboard and binary-difference export, then intraline diff and
  merged-output search. Future sync work must distinguish directional copy from
  newer-file update and baseline-aware bidirectional conflict handling.

## Verified x64 EXE slimming 2026-10-25

- EXE 3151872 -> 1083904 bytes; formal SHA
  13CE079CEFB03C01EDF826B45A74FDCAAE38E9609604885AC44807F5CB67DEA9.
- Canonical Release and Debug PASS. Separate Debug/Release NativeUiTests --workspace (35), --zoom (34), and --workspace-process against each exact formal EXE (11) PASS.
- Independent synthetic documents/workspaces only; original editor INIs/drafts untouched.
- Only x64 product definitions changed; application sources are byte unchanged.
  Previous merge/advanced/core evidence is reused; no unrelated full suite repeated.
- Actual clean removed 186 compiler files; 15 protected actual files SHA
  unchanged, including private INIs/data and delivery/runtime files.
  17 nested fixtures/source, help/idempotence PASS; unchanged junction evidence reused.
- LOCAL ONLY by user instruction; no repository initialization or remote/publication.
- Raw numeric evidence: docs/verification/exe-slimming-20261025.json.


## Latest completed scope: local compare / merge / sync (2026-10-25)

The user selected left/right path fields plus native Browse, and expanded the
scope to folder comparison/synchronization and three-way merge. That accepted
scope is now implemented; earlier gap lists below are historical.

Implemented line-aligned two-way text/source comparison with basic lexical
colors and ignore-space/case/EOL rules; selected-block left/right/base/both
choices, choice Undo/Redo, editable output and atomic output Save. Three-way
Base/Left/Right merges independent changes automatically and guards unresolved
conflicts. Reviewed manual resolution requires removal of marker lines and
explicit acknowledgement; subsequent editing invalidates it. Source tabs/drafts
remain independent. Late save cancellation adopts an actual committed save and
cancels deferred close; cancelling a render restores its prior choices/history.

Implemented recursive on-disk folder content comparison, include/exclude globs,
selected-directory expansion, selected/all copy in either direction, update and
explicit mirror modes. Every sync presents its complete virtual operation list
before applying; Cancel is the default preview button. Overwritten and removed
file contents are retained under the target's `.SimpleEditorTool.backups` tree.
Root/file identities and stale source/target states are rechecked. Reparse paths,
overlapping physical roots, dirty/busy affected editors and conflicting operation
locks are rejected. Locks use paths plus cached file IDs and an operation owner,
so aliases cannot bypass locks or release another operation's locks.

Implemented portable workspace INI Save/Open for all session types, paths/rules,
encodings, zoom and view state. Restore appends/activates and preserves existing
drafts; comparisons await Compare, and no synchronization starts automatically.
Fixed native Edit LF/CR display by adapting to CRLF while retaining original
endings in save/snapshot data. Also fixed native ComboBox prefix autocomplete
expanding a folder path to an existing open child-file path.

Changed implementation files: new `src/core/line_merge.h/.cpp`,
`src/core/file_operations.h/.cpp`, `src/core/folder_sync.h/.cpp`,
`src/core/workspace_session.h/.cpp`, `src/ui/advanced_compare.h/.cpp`,
`src/ui/advanced_compare_actions.cpp`, `src/ui/path_combo.h`,
`src/main_frame_advanced.cpp`, `src/workspace_sessions.cpp`; updated
`src/core/binary_document.h` (cached identity accessor),
`src/main_frame.h/.cpp`, `src/main_frame_paths.cpp`,
`src/main_frame_compare.cpp`, `src/main_frame_edit.cpp`,
`src/main_frame_save.cpp`, `src/main_frame_zoom.cpp`,
`src/workspace_frame.h/.cpp`, `src/resource.h`, `src/simple_editor_tool.rc`.
Updated the three vcxproj files, `tests/native_ui_tests.cpp`, new
`tests/native_merge_sync_tests.cpp` and core test dispatch. Documentation:
AGENTS, SPEC, README, SELF_CHECK and this handoff. Build/clean policy, production
startup handshake, static runtime flags and source document algorithms were not
replaced. No Python source or new runtime installation was introduced.

Actual verification: canonical Release and Debug builds succeeded. Focused
core `--merge-sync` passed 47 assertions, including 300 randomized two-way and
2,000 randomized three-way source reconstruction trials, bounded large-region
fallback, Unicode case, mixed EOL, selected-directory sync, backups, stale plans,
cancel/guard/traversal and malformed workspace parsing. Large interleaved edit
ranges use a linear grouping sweep with cancellation, and newline-heavy inputs
are rejected at the line cap before per-line allocations. UI `--advanced` passed
52 assertions in each Debug/Release. Shared GUI bundles passed 118 assertions
each, including existing editor/search/binary/zoom/async-save behavior; workspace
checks passed 35 each. The broader GUI bundles were selected because shared
frame/layout, editor text representation and save routing changed. Final
range-grouping and bounded newline-conversion refinements reran the affected
core/advanced UI checks and exact final EXE process/startup checks; unchanged
shared GUI/workspace evidence from this pass remains applicable.

Both final formal EXEs passed byte-matched normal/renamed process forwarding,
concurrent single-instance startup, retained file tabs, selected close, normal
exit/relaunch (11 checks each), and isolated fresh no-argument startup/frame/
normal-close smoke with exit 0. No production target was running at build
preflight. Two early test-owned fixtures (PIDs 6156 and 97012) were stopped after
path/argument checks while fixing fixture dialog handling; no user session was
terminated. Final runs completed unassisted. Test-owned native PNG captures of
two-way, three-way conflicts and folders were inspected; the LF rendering and
clipped header/mode labels found there were corrected and recaptured.

Pre-slimming Release (2026-10-25): `build/SimpleEditorTool.exe`, 3,151,872 bytes, SHA-256
`AC5665FCAE1B588AB72BE3447E4903839558AA829B4BEDA0E79B16A5EA60F887`.
Debug SHA-256: `18E008D110FEED76E4CFCD654E927AC8BAD4089A3F91F56F13A4A71C2DC79D92`.
Production imports were inspected: Windows system libraries only; no dynamic
MFC/VC runtime installation dependency. `build` contains exactly this EXE plus
the three original INIs, all INI SHA-256 values unchanged. Logs/screenshots and
the summary manifest are in `artifacts/evidence/advanced-*`. Clean scripts were
unchanged; prior clean-preservation evidence is reused, not rerun.

Limits: text merge 16 MiB / 200,000 lines per input, source preview 8,192 chars
per line, folder scan 200,000 entries, workspace 1 MiB / 200 sessions. Large
repetitive regions may use coarse blocks. Syntax colors are lexical, not full
language parsers. Workspace INIs do not save unsaved buffers. Folder sync is
per-file atomic: cancel/error retains already committed operations and backups;
no automatic whole-folder rollback. Metadata guards are optimistic, not an
exclusive transaction against other apps. No embedded tree was added (the user
selected native Browse), nor remote/archive/plugin parity with commercial tools.
NOT RUN: physical pointer drag/file-picker gestures, manual cross-monitor/DPI
inspection, old-version peer emulation, power-loss/SMB fault injection. Native
control/HDROP/dialog/process tests and actual render captures do not imply those
manual/fault checks. The repository remains local with initially untracked
project files; no commit or publication was requested.

## Earlier implementation history

The project has been migrated to native Visual C++ MFC. Python, PySide6, and
PyInstaller sources and scripts were removed. The portable Release x64 binary is
`build\SimpleEditorTool.exe`; it uses static MFC and static CRT linkage.

Implemented: virtualized hex/ASCII view, hexadecimal overwrite editing,
piece-table insert/delete, undo/redo, atomic save, editable text, asynchronous
insertion-aware binary comparison with differing addresses and values,
drag-and-drop, command-line open, single instance, custom icon, and maximized
startup.

The first post-migration improvement pass added caret-based insert/delete,
Home/End/arrow/Page navigation, unsaved-change protection, selectable
equal-address comparison, progress/cancellation, and an owner-data difference
list that avoids constructing up to one million UI rows. Double-clicking a
result navigates to its left-file address.

Text mode is now editable for documents up to 32 MiB. Saving routes the edited
text back through the piece-table and atomic-save path, preserving UTF-8 and
UTF-16 byte-order marks. Larger documents remain read-only in Text mode and are
edited through the virtualized Hex view.

Compare mode now contains side-by-side read-only Hex panes backed by separate
documents, synchronized vertical scrolling, full path labels, and two-sided
navigation from the virtual difference list. Comparison requires saving pending
left-side edits first so the displayed data and background disk comparison stay
consistent.

Verification: Release x64 and Debug x64 build with zero warnings/errors; native
core tests pass; command-line file-open smoke test succeeds; Windows reports
startup placement `SW_SHOWMAXIMIZED`.

## Completed recommendation pass (2026-10-24)

All previously suggested items are implemented: text/hex Find and Replace,
Replace All, Go To Address, previous/next difference, both draggable comparison
splitters, and the encoding selector. Shortcuts and input syntax are in README.

`src/core/search.*` performs streaming searches. `text_codec.*` rejects invalid,
NUL-containing, or lossy text conversions. CP950 search respects character
boundaries; UTF-16 searches stay aligned. Replace All builds a new piece table
on a snapshot and preserves one-step Undo/Redo. Find does not copy edit history.

`main_frame_edit.cpp` and `main_frame_compare.cpp` separate commands from frame
layout. A single background future handles search, replace-all, and compare;
cancellation or document revision changes discard the result. UI polling gets
only completed work, with no UI-thread worker join. Text/Hex tab switches now
commit/refresh the same document. Edits invalidate old comparison addresses.
Save As adopts its destination, and a failed Open preserves the current file.

`scripts/build_release.bat` is the canonical delivery entry point. Debug output
is isolated in `build/debug`, so it cannot overwrite the Release executable.
The build preflight found no running target instances requiring closure.
Test-owned smoke instances were closed normally.

New verification passed: native core tests; real MFC control/dialog tests in
Debug and Release; one-million-row comparison, truncation indication and last
row navigation; splitter dragging; text/hex synchronization; cancellation and
stale-result rejection; CP950 boundary search; maximized startup/open/shutdown
for both application builds; system-only PE dependencies. Clean audit preserved
22 actual build files by SHA-256, checked 17 fixture files and junction rejection,
and removed 124 reproducible compiler files. No user/delivery files were removed.

Historical recommendation-pass SHA-256: `BF7C40FB682BE2B679A7D4C99E8B65ADC7E7E989687466D98988B7FEB9167BE2`.

Limits: literal case-sensitive searches, text editing up to 32 MiB, comparison
display limited to the first 1,000,000 differences with an explicit label.
NOT RUN: manual visual inspection at 125% DPI and cross-monitor DPI changes.
The native tests exercise controls and messages, not physical mouse automation.

## Changed files and purpose

- `src/main_frame.h`, `src/main_frame.cpp`, `src/main_frame_edit.cpp`, `src/main_frame_compare.cpp`: command routing, page synchronization, safe background jobs, difference navigation and splitter layout.
- `src/hex_view.h`, `src/hex_view.cpp`: measured font geometry, accurate hit testing, search/difference highlighting, horizontal scrolling and synchronized keyboard scrolling.
- `src/core/binary_document.h`, `src/core/binary_document.cpp`: snapshots, batch replacement with grouped undo/redo, safe failed Open and Save As adoption.
- `src/core/search.h`, `src/core/search.cpp`, `src/core/text_codec.h`, `src/core/text_codec.cpp`: streaming matching, 64-bit address/hex parsing and lossless encoding rules.
- `src/ui/editor_dialogs.h`, `src/ui/editor_dialogs.cpp`, `src/ui/splitter_bar.h`, `src/ui/splitter_bar.cpp`, `src/resource.h`, `src/simple_editor_tool.rc`: Find/Replace and address dialogs, splitter controls and command resources.
- `src/services/editor_job.h`: background result ownership, isolated from visible UI data.
- `tests/native_core_tests.cpp`, `tests/native_features_tests.cpp`, `tests/native_ui_tests.cpp`, `NativeCoreTests.vcxproj`, `NativeUiTests.vcxproj`: core regressions and isolated native MFC control tests.
- `SimpleEditorTool.vcxproj`, `scripts/build.ps1`, `scripts/build_release.bat`, `.gitignore`: native feature build inputs, one-click build, separate Debug output and compiler-symbol ignore rules.
- `AGENTS.md`, `SPEC.md`, `README.md`, `SELF_CHECK.md`, `HANDOFF.md`: feature contracts, operation instructions, affected verification and actual results.

To reproduce, use the build/test entry points in README. The native UI tests
must run sequentially when exercising mouse capture and modal dialogs.

## File safety and comparison reliability pass (2026-10-24)

Source handles now record file ID, length and last-write time. Reads and snapshots
reject detected in-place changes, and Save checks the original path and the
destination again before commit. An externally replaced or deleted source path
cannot be silently overwritten/recreated. If the retained original handle is
still valid, Save As can recover the editor's bytes without touching the new
external file. Read failures leave the destination and edit history intact.

An existing destination uses a flushed sibling temporary plus `ReplaceFileW`
with a temporary backup. Initial regression tests caught `MoveFileEx` returning
Win32 5 for an opened destination; this was fixed using the documented native
replacement operation. Failed replacement retains the original source handle;
partial replacement attempts backup restoration and otherwise reports retained
recovery paths. Successful saves adopt and validate the written document and
remove the transaction backup. The implementation follows the
[Microsoft ReplaceFileW contract](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew).

Comparison now reads independent snapshots of the same document versions used
by the panes. Every read is checked; mid-run truncation exits with an error.
Aligned and equal-address results are batched at no more than 4096 rows, with
cooperative cancellation inside scan/anchor loops. Empty files finish at 100%.

File > Reload (`Ctrl+R`) uses normal unsaved-change confirmation. Cancel retains
edits; confirming discard reopens current disk content. Periodic source-state
checks invalidate stale comparison rows after either input changes. Hex painting
reads only one visible block rather than allocating and reading every row.

Changed files: `src/core/binary_document.h/.cpp`, `src/core/diff_engine.h/.cpp`,
`src/main_frame.h/.cpp`, `src/main_frame_compare.cpp`, `src/hex_view.cpp`,
`src/resource.h`, `NativeCoreTests.vcxproj`, `tests/native_core_tests.cpp`, new
`tests/native_file_safety_tests.cpp`, `tests/native_ui_tests.cpp`, and the project
AGENTS/SPEC/README/HANDOFF/SELF_CHECK documents. Build and clean scripts are
unchanged in this pass.

Verification: native core bundle passed because shared reads affect editing,
search and comparison; the follow-up `--file-safety` run passed after adding the
backup cleanup assertion. Debug and Release MFC tests passed, including Reload
Cancel/discard, externally replaced sources, in-place changes and stale right
comparison invalidation, plus previous million-row and editing regressions.
Production Debug and one-click Release builds succeeded without warnings/errors
in the output. Both EXEs opened README maximized and exited normally. Build
preflights found no running target requiring closure; smoke PIDs 44512 (Release)
and 96324 (Debug) were test-owned and closed gracefully. Release dependencies
remain Windows system components only. Earlier single-instance forwarding and
unchanged clean-script preservation evidence was reused, not rerun.

Historical file-safety Release: `build/SimpleEditorTool.exe`, 2,822,144 bytes, SHA-256
`91C1077802923F22EE26C351F41743730921A45CFB4D723E08761A6BFB712BFC`.

Limits: metadata detection is optimistic, not a cryptographic check or a lock
against simultaneous external writes. At this earlier pass, Save still wrote the
whole file on the UI thread; the background-save pass below resolves that I/O limitation.
NOT RUN: power-loss/disk-full fault injection, rare ReplaceFile partial-failure
codes, SMB/network filesystems, and manual 125%/cross-monitor DPI inspection.

## Background Save and deferred actions (2026-10-24)

Save/Save As now write, flush and replace a document snapshot on the single
background job slot. The UI polls progress; no save I/O or worker join runs in
the UI command handler. Hex/Text changes and modal navigation are paused during
Save, while scroll, resize and tab viewing remain available. Completion adopts
the clean saved document without resetting the caret. Failure keeps the original
document/history and restores editing.

Core `SaveControl` checks cancellation at chunk boundaries and immediately
before replacement. Committed saves win over late cancellation, so the UI never
discards an already-written snapshot merely because the cancel flag is set.
Cancel also clears deferred actions. Save-before-Open/Close/Compare resumes only
after success. Open-dialog/Reload/Close requests received during Save are queued;
the latest request is retained. Close is posted after completion to avoid
destroying the MFC frame while the result callback is active. File-state polling
skips an in-flight Save so the tool does not mistake its own replacement for an
external write.

Changed files: `src/core/binary_document.h/.cpp` (SaveControl/checkpoints),
`src/services/editor_job.h` (save result), `src/main_frame.h/.cpp`,
`src/main_frame_edit.cpp`, `src/main_frame_compare.cpp`, new
`src/main_frame_save.cpp` (save ownership/continuations),
`SimpleEditorTool.vcxproj`, `NativeCoreTests.vcxproj`, `NativeUiTests.vcxproj`,
`tests/native_core_tests.cpp`, new `tests/native_async_save_tests.cpp`,
`tests/native_ui_tests.cpp`, and AGENTS/SPEC/README/HANDOFF/SELF_CHECK.
Build/clean scripts, delivery layout, runtime linkage and instance locking are unchanged.

New checks passed: `NativeCoreTests.exe --async-save` and `--file-safety`;
Debug/Release full native MFC runners, including background Save, read-only edit
guards, scrolling/caret retention, Save As, failed-save retries, pending Open and
Close on success/failure, pre/post-commit cancellation, Reload after Save, and
native file pickers appearing only after a successful save. UI fixtures verify
safe MFC destruction by HWND/permanent-object identity and process exit.
Previous edit/search/comparison/million-row cases also passed as affected callers.

One-click Release and separate Debug builds passed without compiler warnings or
errors in their output. Both production EXEs opened README maximized and exited
normally. Build preflights found no running target requiring closure. Smoke PIDs
91360 (Release) and 93720 (Debug) were test-owned and closed gracefully. PE imports
remain Windows system components only; earlier clean preservation and production
single-instance evidence was reused rather than rerun.

Historical background-save Release: `build/SimpleEditorTool.exe`, 2,834,944 bytes, SHA-256
`4424768E6F3CCE534D0684C6168C61744A26F38851265EEEEBF2ABA5C9864D95`.

Limits: text encoding/snapshot preparation remains on the UI thread. Cancellation
is cooperative and cannot interrupt an OS write/flush already in progress.
NOT RUN: abrupt power-loss/disk-full/native partial-replacement fault injection,
SMB/network latency, physical gesture automation and manual 125%/cross-monitor DPI.

## Delivery directory separation (2026-10-24)

`build` now contains only `SimpleEditorTool.exe`, `simple_editor_tool.ini`,
`test.ini` and `ui_smoke.ini`. Development subdirectories, linker PDBs and the
three screenshots were moved out of delivery. All 33 original files were
SHA-256 verified after relocation, before rebuilding; no original INI was
replaced. The three INI hashes also match after all builds, smoke tests and clean.

Changed files: `SimpleEditorTool.vcxproj`, `NativeCoreTests.vcxproj`,
`NativeUiTests.vcxproj` (output/intermediate/PDB/import-library paths),
`scripts/build.ps1` (Debug path and delivery-content check),
`scripts/clean_intermediates.ps1` (allowlisted clean under `artifacts/obj`),
`.gitignore`, `AGENTS.md`, `SPEC.md`, `README.md`, `HANDOFF.md`, `SELF_CHECK.md`.
Application source, compiler optimization, security options and runtime linkage
were unchanged. Release remains Unicode x64, static MFC/static CRT, v143 with
VS 2022 MSBuild 17.14.18 and MSVC tool directory 14.44.35207.

Canonical paths superseding earlier sections: delivery `build`; Debug EXE
`artifacts/debug`; test EXEs `artifacts/tests`; compiler/linker PDBs and import
libraries `artifacts/obj`; screenshots/inventory `artifacts/evidence`.
The preserved baseline EXE is `artifacts/baseline/layout-before/SimpleEditorTool.exe`.
`artifacts/evidence/build-layout-before.json` and `build-layout-moves.json`
record the original inventory and verified relocation. Reproducible symbols
may be removed by normal clean and regenerated by building.

Verification passed: one-click Release and production Debug builds, Release
core build plus `--file-safety`, Debug/Release UI builds, Debug UI
`--async-save`, and isolated hash-matched Release/Debug startup/frame/normal-close.
Final Release was rebuilt after actual clean and separately smoke-tested.
No compiler warnings/errors were reported. Release imports remain Windows system
components only; no additional installable runtime was introduced.

Clean verification passed: shared static scan of three scripts, non-destructive
help, idempotence and 17 protected fixture files, junction rejection before
deletion, actual-project preservation of 15 protected build/object-tree files,
and an additional 11 non-intermediate artifact files. Actual clean removed 135
allowlisted compiler files. Evidence, unknown files, EXEs and INIs survived.

Historical delivery-layout Release: 2,834,944 bytes; SHA-256
`796ADBFA1773608206ECA510716ECAFECE98A0BA3DEDF4D0663BFD286982B783`.
EXE size is unchanged from the preserved baseline; the complete existing EXE
plus three INIs totals 2,835,667 bytes before and after. Root directory file
contents fell from 56,034,371 to 2,835,667 bytes by relocating development files.
This separates delivery contents; it does not claim smaller linked code or a
runtime performance gain. No compiler/runtime/resource optimization was attempted,
so paired throughput/performance benchmarking is N/A for this path-only change.
Unchanged editing/search/comparison/single-instance evidence is reused; full
regression suites and manual DPI inspection were not rerun for output relocation.

## Per-tab wheel zoom (2026-10-24)

Hex, Text and Compare now support plain wheel content zoom: 10% per notch,
50%-300%, accumulating partial deltas from high-resolution wheels. Each tab
retains its scale during the session; Compare shares one scale across both Hex
panes and its difference list. A separate right status pane displays `Zoom: N%`
and stays enabled during idle updates, independently of progress/error messages.
Shift+wheel uses ordinary content scrolling; Ctrl+0 resets the active tab and
its partial wheel accumulator. No INI key or restart persistence was added.

The reusable child-control wrapper handles native wheel messages; frame
pretranslation also routes wheel input over the page when focus remains on the
tab strip. Search-dialog input remains isolated. Hex remeasures glyph width,
row height and scroll ranges so zoomed byte hit testing stays aligned. Text
changes its borrowed font without replacing the text buffer or resetting Undo,
selection or dirty state; its first visible line is retained. Compare scales
the virtual list/header font and measures a fixed set of header/address samples
for column widths, without scanning difference rows. Zoom remains available
during background Save and does not mutate the saved snapshot.

Changed implementation files: `src/main_frame.h/.cpp`, new
`src/main_frame_zoom.cpp`, new `src/ui/wheel_zoom_control.h`,
`src/hex_view.h/.cpp`, `src/resource.h`; build entries in
`SimpleEditorTool.vcxproj` and `NativeUiTests.vcxproj`; focused tests/filter in
`tests/native_ui_tests.cpp`; documentation in `AGENTS.md`, `SPEC.md`, `README.md`,
`HANDOFF.md`, `SELF_CHECK.md`. No core file/codec/diff/save algorithm, compiler
flags, framework/runtime dependency, or clean script changed.

Verification: production Release and Debug builds passed. Both configurations
passed `NativeUiTests.exe --zoom` (34 checks each), including partial wheel
accumulation, focused-tab routing, min/max scales, per-tab retention, Hex hit
testing, Text draft/selection/scroll/Undo retention, comparison synchronization
and columns, independent status updates, Ctrl+0, live borrowed fonts and 30
cycles of resize/repaint/tab/zoom without GDI font growth. Both configurations
passed `--async-save`, including the added wheel-during-save assertion and the
existing save/error/cancel/deferred-navigation checks. A test-only compile error
using private `WM_IDLEUPDATECMDUI` was corrected to the public status-bar update
method; subsequent focused builds/runs passed.

Final hash-matched isolated Release/Debug copies created an owned MFC frame and
closed normally, exit 0. Fixture directories are recorded in
`artifacts/evidence/zoom-verification.json`. No exact build target needed closing.
All three original INI hashes remain unchanged; `build` still contains only the
EXE and those INIs. Earlier clean-preservation, single-instance and unrelated
core/full-suite evidence was reused rather than rerun.

Historical wheel-zoom Release: `build/SimpleEditorTool.exe`, 2,845,696 bytes, SHA-256
`15AEE9A411112C9C24904C9550B7845A9B26A7316A3CE8B01FF6DF244C248489`.
NOT RUN: physical wheel gestures and manual 100%/125%/cross-monitor DPI visual
inspection. Native control messages and font geometry were verified; the
computer-use `node_repl` tool is not available in this session. No manual/physical
interaction or DPI coverage is inferred from native message tests.

## Retained file sessions and comparison paths (2026-10-24)

Production startup now creates `CWorkspaceFrame`, with independent child editor
sessions. Every dropped/Open/forwarded file appends a tab or activates its existing
path; it never repurposes another editor. Each session keeps its actual native
Text control, draft/Undo, BinaryDocument, caret, encoding, zoom and background job.
Ctrl+W / Close tab closes only that session, while Ctrl+Tab cycles sessions.
Root commands and shortcuts route to the active editor, including toolbar focus.
Source Reload/Save refresh the workspace's title/dirty labels.

Compare files creates a separate read-only comparison session with editable
left/right paths, current-open-file dropdowns, native Windows file Browsers,
Swap sides, comparison mode and Compare/Recompare. No left file must be opened
first. Unsaved native Text content is encoded into a private snapshot without
committing/resetting the source buffer or Undo; Hex edits also remain independent.
Changing paths invalidates old results/source views. Returning to a comparison
refreshes path choices without discarding its completed result.

Closing an inactive/saved tab preserves the user's active editor. Save failures
and cancellation abort pending workspace exit. New Open requests cancel queued
exit continuation; reopening a pending save-and-close tab cancels its close while
letting the save finish. Saved-close messages are cancellable and posted, retaining
the prior safe MFC destruction boundary. Closing an editor releases its modeless
search dialog before destruction. Reentrant close prompts are guarded.

The existing session-local singleton remains. Secondary/renamed launches wait
for the primary workspace to publish readiness and forward all paths with a
bounded multi-string payload and bounded message timeout. A workspace-version
property avoids forwarding new requests into an old destructive single-document
instance. Only the workspace publishes normal production window identity.

Changed files: new `src/workspace_frame.h/.cpp`, new `src/main_frame_paths.cpp`,
`src/app.cpp`, `src/main_frame.h/.cpp`, `src/main_frame_compare.cpp`,
`src/main_frame_edit.cpp`, `src/main_frame_save.cpp`, `src/resource.h`,
`SimpleEditorTool.vcxproj`, `NativeUiTests.vcxproj`,
`tests/native_ui_tests.cpp`, new `tests/native_workspace_process_tests.cpp`,
and `AGENTS.md`, `SPEC.md`, `README.md`, `HANDOFF.md`, `SELF_CHECK.md`.
Core edit/diff/save/codec algorithms, compile/link optimization, static MFC/CRT
contract and clean/output-layout scripts did not change.

Verification passed: production one-click Release and Debug builds; final
Debug/Release `--workspace` (35 checks each) and `--async-save`. The shared full
GUI bundles also passed in both configurations during this pass because the
frame/menu/compare layout changed; later close-state corrections reran the
affected workspace/save cases. Existing million-row comparison, search/replace,
source safety, splitters and zoom checks passed in those bundled GUI runs.

Final formal Release and Debug binaries also passed the process runner: source
and renamed isolated copies match every EXE byte; concurrent launches produce
one workspace; all argument paths append/activate correctly; Close tab retains
other files; normal exit/relaunch releases the singleton. Both exact formal
binaries separately passed fresh no-argument startup/frame/normal-close, exit 0.
No exact production target needed closing before build. An early fixture stalled
on its missing-file message-box dismissal; only test-owned PID 96080 was stopped.
The fixture now finds its own thread's dialogs and closes error boxes independently
of localized button IDs; final runs completed without manual dismissal.

Historical Release: `build/SimpleEditorTool.exe`, 2,877,952 bytes, SHA-256
`ED1AD9C8591001B13DBB5130241ECA9A36D9E9948F63D705CA9D62AA8A03FB8D`.
Debug SHA-256: `BF9C59A00130BEAC5B1A5626D28633FB37D10089B35CBDAF805DD10D175282FA`.
Manifest/results are in `artifacts/evidence/workspace-verification.json`.

Historical scope (superseded by the latest completed scope above): retained editor/comparison sessions and binary comparison
path workflow, with native Explorer Browse dialogs. No embedded folder tree was
added. The broad WinMerge/Beyond Compare request still has explicit feature gaps
in SPEC's alignment table: line-aligned text/source diff, syntax rules, two-/three-way
merge, recursive folder comparison/synchronization, and persistent session restore.
Do not describe this build as full feature parity. The optional scope questions
received no answer during this pass; the concrete requested file-retention and
dual-path workflow was completed using the stated native-Browse default.

NOT RUN: physical Explorer drag/click gestures, positive file-picker selection
by desktop input, manual 100%/125%/cross-monitor DPI inspection, old-binary peer
emulation, power-loss/SMB fault injection. Synthetic native HDROP, both Browse
dialogs' open/Cancel, typed path processing, native controls and actual
formal-process forwarding were verified. Clean scripts were unchanged, so prior
clean-preservation evidence is reused rather than rerun.
