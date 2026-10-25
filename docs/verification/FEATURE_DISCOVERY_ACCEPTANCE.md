# Approved feature-discovery acceptance — 2026-10-25

All twelve approved first versions are implemented and accepted. This record
describes changed components and verification, not full competitor parity.
Product instructions: [workflows](../EDITOR_AND_COMPARE_WORKFLOWS.md),
[tables](../STRUCTURED_DATA.md). Numeric delivery identity and counts are in
[discovery-20261025.json](discovery-20261025.json).

## Changed components and purpose

All source paths below are relative to `src/`.

| Row | Source files / integration | Result and relevant evidence |
| --- | --- | --- |
| R01 | `core/hex_bookmarks.*`, `hex_view.*`, `main_frame_features.cpp` | Named/color/comment ranges, native list/remove/jump/highlighting, stale guards and workspace persistence; discovery core/UI, actual range-list capture. |
| R02 | `core/decimal_compare.*`, `core/table_compare.*`, `core/structured_compare.*`, `ui/advanced_structured.cpp`, `ui/advanced_feature_settings.cpp` | Keyed/reordered rows, explicit column mappings and exact decimal tolerances; duplicates/missing keys/invalid numeric fields remain explicit. Core and native recompare/persistence checks. |
| R03 | `core/compare_rules.*`, `core/line_merge.*`, `core/text_report.*`, `ui/advanced_compare_report.cpp`, `ui/advanced_feature_settings.cpp`, `ui/advanced_compare.cpp` | Two-way literal ignores/timestamp masks and report annotations preserve raw text; core/rule-report and native source/output checks. |
| R04 | `ui/text_commands.*`, `main_frame_text_commands.cpp` | Bounded background sort/deduplicate/blank removal/trailing trim with native single Undo. Discovery UI and affected search/Undo tests. |
| R05 | `services/batch_replace.*`, `ui/batch_replace_dialog.*`, `workspace_batch_replace.cpp` | Disk-only preview/select/apply with original codec/EOL, metadata+SHA-256 guards and existing atomic-save/backup/journal infrastructure. Native success/negative/cancellation/recovery/dirty-editor cases. |
| R06 | `ui/live_inspector.*`, `main_frame_features.cpp` | Caret-following numeric values and explicit endian/width-safe writes; stale revisions cannot redirect a typed value. Native write/one-transaction/stale checks and actual capture. |
| R07 | `core/manual_alignment.*`, `ui/advanced_compare.*`, `ui/advanced_compare_actions.cpp`, `ui/advanced_compare_report.cpp` | Validated monotonic anchors segment a read-only two-way comparison; no merge output/actions, clear/recompare restores merging. Core/native pairing and normal merge regression checks. |
| R08 | `main_frame_clone.cpp`, `main_frame_editor.cpp`, `main_frame_edit.cpp`, `main_frame_zoom.cpp`, `main_frame.cpp`, `workspace_preferences.cpp` | Two Scintilla views share one buffer/Undo/save and retain independent navigation. Native shared edits/Undo/save locks, theme/font routing and split-layout capture. |
| R09 | `ui/text_commands.*`, `ui/code_editor.h`, `main_frame_text_commands.cpp`, `main_frame_search.cpp` | Bounded prefix/document/keyword completion rejects stale buffers/carets. Native popup after shared-view Save As uses the owner's current language; typing during planning retains the draft and rejects old results. |
| R10 | `core/rename_pairs.*`, `ui/advanced_content_pairs.cpp`, `ui/advanced_compare.*` | Read-only size/SHA-256 pairing, duplicate ambiguity and version guards. Core candidate/cancel/stale cases and native pair selection opens a separate binary comparison without renaming. |
| R11 | `core/log_tail.*`, `ui/log_tail_dialog.*`, `main_frame_features.cpp` | Independent bounded saved-disk UTF-8/UTF-16 follower handles append/truncate/rotation/pause/close. Core version handling and native dirty-draft preservation/shutdown/capture. |
| R12 | `core/crc_profiles.*`, `main_frame_crc.cpp` | Streaming named/custom 8–64-bit CRC, selected/whole source, profile persistence. Five preset vectors, custom CRC-17/CAN-FD, cancellation/bounds and actual native worker check. |

Shared integration changes: `main_frame.h`, `main_frame_compare.cpp`,
`main_frame_save.cpp`, `workspace_frame.*`, `workspace_sessions.cpp`,
`core/workspace_session.*`, `services/editor_job.h`, `resource.h`,
`simple_editor_tool.rc`, and new `ui/feature_dialogs.*`. The product/core/UI
projects register their respective new sources; no library dependency was added.
Optional workspace fields remain backward compatible with older version-1 files.

Tests: `tests/native_discovery_tests.cpp`,
`native_discovery_analysis_tests.cpp`, `native_discovery_ui.inc`,
`native_discovery_flows_ui.inc`, `native_discovery_visual_ui.inc`, and the existing
core/UI dispatchers. Product docs, SPEC, local AGENTS, active ledger, HANDOFF and
SELF_CHECK were updated to reflect the available behavior.

## Actual verification

Core Release checks: discovery **20**, discovery-analysis **25**, affected
text-reports **22**, merge-sync **47**. Debug and Release native UI checks each:
discovery **19**, discovery-flows **19**, discovery-visual with captures **17**,
discovery-completion **3**, advanced **52**, workspace **35**, search-tools **32**,
recovery **13**, async-save **32**, zoom **34**, text-reports **13**. All passed.
The final completion-only correction reran discovery/completion in both
configurations and visual checks in Debug; unaffected focused results are reused.
Other earlier algorithm/format/firmware tests were not rerun as a full suite.

In particular, batch replacement actually verified UTF-8 BOM/CRLF and UTF-16
BOM/LF preservation, skipped invalid input, untouched preview inputs,
same-size/time changed-content refusal before commits, dirty editor refusal,
cancellation after one commit and journal recovery of that committed file.
The shared-view completion fixture uses explicit end-of-buffer positioning;
`SetSel(-1,-1)` means deselection and must not be used to assume an EOF caret.

Canonical `scripts/build_release.bat` and
`scripts/build.ps1 -Configuration Debug` passed without compiler/linker warnings
or errors. Final Release is **3,271,680 bytes**, SHA-256
`41C83EACD05C2B0FD7097212B14A98A8136D9729447B92D76CCAE0A1EE7AABCA`.
The previous approved-scope baseline was 3,035,136 bytes. There is no EXE size
optimization claim for this feature addition.

Byte-matched formal EXE copies in each configuration passed **12 GUI process
checks** and **7 CLI cases**: startup maximization, concurrent/renamed singleton,
retained multi-file forwarding, selected close, clean shutdown/relaunch,
headless reports/exit status/overwrite guards and unchanged isolated GUI INI.
PE inspection confirms x64, ASLR/high-entropy/NX, and Windows system imports;
static MFC/CRT/editor, no Python/.NET/Qt or extra runtime/driver dependency.

Clean verification passed: **99** scripts in the static scan, **17** protected
fixture files, idempotence and junction rejection. Actual project clean removed
**663** allowlisted compiler intermediates and preserved **18** protected actual
files by SHA-256. `build/` contains exactly EXE plus the original three INIs;
their original hashes are unchanged. Six actual native feature captures per
configuration were visually reviewed under `artifacts/screenshots/discovery-*`.
Raw build/test/audit logs are local under `artifacts/logs/discovery*` and the
affected `*-ui-{debug,release}.log` / core logs.

## Reproduction

Build core/UI test projects with MSBuild, x64, Release for core and Debug/Release
for UI. Run `NativeCoreTests.exe --discovery`, `--discovery-analysis`,
`--text-reports`, `--merge-sync`. For UI run `NativeUiTests.exe` with each mode
listed above; `--discovery-visual <screenshot-directory>` also captures controls.
Use `--workspace-process <formal-exe>` and
`scripts/verify_cli.ps1 -Executable <formal-exe>` sequentially for each artifact.
No unrelated normal application instance may own the process-test singleton.
After the preservation audit, the next build regenerates allowlisted objects.

## Explicit limits and coverage

- Manual anchors are a read-only two-way view. Clear/recompare before merge or
  report export; advanced literal ignore rules do not change three-way conflicts.
- Completion uses native keyword dictionaries (dedicated Python/JSON and common
  C-family fallback), not a language server or complete lexer dictionaries.
- Batch replacement commits per file, not atomically for a whole folder. Failed
  or cancelled operations can retain private staging; backups/journals remain.
- Log following reads saved disk content, supports UTF-8/UTF-16 and retains a
  bounded window. Legacy code-page random-offset following is not offered.
- All bounds, staleness and persistence behavior are in the product guides.
  Native per-monitor DPI transitions, physical clipboard/drag/wheel/multiple
  monitors, real network servers and clean-PC installation are **NOT RUN** here.
  Synthetic message/isolated/native fixtures do not imply physical coverage.
- No GitHub remote, commit or upload was created; publication remains deferred.
  No remaining implementation or delivery item exists in the approved twelve-row
  first-version scope.
