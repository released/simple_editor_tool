# Project Instructions

## Verified x64 size configuration (2026-10-25)

- Only the product x64 project defines _AFX_NO_MFC_CONTROLS_IN_DIALOGS after a
  standard/common/custom control review. Reassess before introducing CMFC controls.
- Keep static runtime, existing optimization/security, architecture and all actual
  editor/export/parser/quota features; never remove required dynamic-load sidecars.
- Use isolated synthetic sources and hash-matched Release/Debug app checks. Keep single-instance tests isolated from unrelated normal instances.
- Reuse unchanged core/advanced-algorithm evidence for framework-only changes; test the affected native GUI paths.
- Scope and raw measurements: SELF_CHECK.md and docs/verification/exe-slimming-20261025.json.


## Purpose

This is a native Windows MFC text, hexadecimal, and binary comparison editor.

## Local Rules

- Use Visual C++ MFC, Unicode, x64, static MFC, and static CRT (`/MT`).
- Active implementation ledger: `docs/NATIVE_WORKFLOW_IMPLEMENTATION.md`. All nine approved N01-N09 rows are complete; final evidence is in docs/verification/NATIVE_WORKFLOW_ACCEPTANCE.md. `docs/FEATURE_DISCOVERY_IMPLEMENTATION.md` and `docs/ALIGNMENT_IMPLEMENTATION.md` record earlier completed scopes.
- Native Scintilla/Lexilla sources are pinned under vendor and statically built by NativeEditor.vcxproj. Preserve UTF-16 MFC bridge semantics, original encoding/EOL, independent buffers, guarded saves and native Undo. Keep licenses available in the standalone EXE.
- Hex selection/clipboard mutations must use BinaryDocument transactions and respect read-only/save/sync locks. Streaming reports must remain independent of the virtual result-list limit and cannot overwrite their inputs.
- Scintilla command events are disabled: only real insert/delete notifications are bridged to EN_CHANGE. Fold/bookmark/lexer notifications must never dirty a document or invalidate a worker. Search planning uses isolated hidden native controls on the background job and checks buffer generation before applying.
- Firmware tools validate checksums, address maps and overlaps; binary output must preserve explicit sparse-fill behavior and stay separate from its input. Analysis, extraction and pattern scans remain bounded/streamed.
- Binary differences carry alignment anchors independently of missing-byte addresses. Preserve anchors through range rebasing and ignored-span filtering so subsequent selected merges retain correct offsets. Reports use the same range/ignore settings; display bases remain separate from physical file offsets.
- Diff presentation never rewrites source/merged buffers. Keep bounded Unicode spans, source line indices, cancellation and native Undo when changing similarity/filter/overview views.
- Text patches validate normalized source hashes (when present), exact hunk positions/context/counts and final-newline markers before preview/application. Header paths never choose output files. Keep one Undo action and preserve original disk files until explicit Save.
- Do not introduce Python, Qt, .NET, drivers, or separately installed runtimes.
- Keep file editing and comparison logic in `src/core`.
- Never load an entire large binary merely to render visible hex rows.
- Preserve piece-table editing and atomic replacement saves.
- Long comparisons must remain off the UI thread.
- Preserve maximized startup, single instance, command-line opening, and drag/drop.
- The workspace owns file/comparison session tabs and the published main-window identity. Opening, dropping or forwarding files must append or activate sessions, never replace another session or discard its draft/Undo.
- Keep document controls, edit history, jobs, encoding and zoom owned by each editor session. Close only the chosen session after its unsaved-change decision; failed/cancelled saves abort pending workspace exit.
- Comparison sessions have independent left/right path selectors and read-only snapshots. Include unsaved Text/Hex content from open editor sessions without committing or replacing their native buffers.
- Advanced comparison modes provide two-way text merge, Base/Left/Right three-way merge and recursive folder synchronization. Keep their editable output separate from source snapshots, and never silently save unresolved conflicts.
- Folder synchronization requires an operation preview, source/target revalidation and recoverable overwrite/removal backups. Reject reparse paths and overlapping roots; lock affected open editors by path and file identity for the operation's owner.
- Native Edit controls need CRLF for display. Preserve original LF/CRLF/CR endings through the display adapter, including snapshots and saves; viewing text must not rewrite its newline style.
- Workspace INI files contain paths/view settings, explicit comparison rules and range annotations, never unsaved file/merge buffers. Loading appends or activates sessions, preserves existing drafts and does not automatically execute comparisons or synchronization.
- Text and Hex share one document; commit text before leaving the Text page.
- Run searches/replacement planning, comparisons and save I/O on the background job slot.
- Reject results after document changes; keep test windows outside the production instance identity.
- Search/codec rules belong in `src/core`; dialogs and splitters belong in `src/ui`.
- Validate original file identity/size/write time before reusing source bytes or saving over its path; detected conflicts retain edits and require Reload or a safe Save As.
- Compare the opened document snapshots, check every read, and bound callback batches. Never treat a short read as equal data or EOF progress.
- Use the native replacement transaction for existing destinations and retain recovery files if restoration cannot finish.
- During Save, pause document mutations and modal navigation; preserve caret/scroll. Resume queued Open/Reload/Compare/Close only after a successful save.
- Cancellation is effective only before replacement. Adopt a committed save even if cancellation arrived late; cancel pending navigation/close when cancellation is requested.
- `build` is delivery-only: EXE and existing INI files, with no development subdirectories. Symbols/intermediates go under `artifacts/obj`; Debug/test/evidence output stays under `artifacts`.
- All of `/artifacts/` is local-only. Successful/failed CLI fixtures clean their own copies unless `-KeepArtifacts` is explicit. Use `purge_artifacts.ps1` preview then `-Apply` for reviewed generated output, retaining unknown files/runtime dependencies. Ordinary clean stays conservative. Raw evidence may be pruned after compact verification records are retained; historical artifact paths are not permanent files.
- Preserve per-tab content zoom and its independent percentage pane; zoom must not mutate documents or replace text buffers. Keep borrowed control fonts alive until replaced or destroyed.
- Ordinary mouse wheel scrolls content; only Ctrl+wheel zooms. Apply this to file/shared Text, Hex, comparison results/output and the large-text pager. Keep Ctrl+0 and percentage display.
- A clamped/no-op Hex scroll must not redraw scrollbars, invalidate content or notify synchronized panes. Actual viewport movement repaints without background erasure; resizing/zoom/content edits retain their own invalidation.
- Current delivery/Explorer/Hex-boundary evidence: `docs/verification/explorer-20261026.json`. Earlier modifier/feature EXE hashes are historical.
- Explorer integration is an explicit current-user static verb under HKCU Software/Classes. Never change defaults/HKLM or remove an unowned entry. Preserve registry values on failed updates; keep quoted executable/input paths and existing session forwarding. Test temporary registry roots and isolated, byte-matched EXEs; preserve user INIs.

## Verification

- Select verification by changed behavior and affected state paths.
- Use `scripts/build_release.bat` for the portable Release output. Debug output is separate in `artifacts/debug`; test executables are in `artifacts/tests`.
- For search/edit/codec changes, build and run `NativeCoreTests.vcxproj`.
- For focused file-state/save/compare regressions, run `NativeCoreTests.exe --file-safety`; changes to shared document reads also affect editor/search callers.
- For line merge, newline display conversion, folder synchronization or workspace-file parsing, run `NativeCoreTests.exe --merge-sync`. Use `NativeUiTests.exe --advanced` for the connected controls, conflict choices, output saves, sync preview/locks and session restore; optionally supply an artifacts screenshot directory.
- For save progress/cancellation/commit-boundary checks, run `NativeCoreTests.exe --async-save` and `NativeUiTests.exe --async-save` in affected configurations.
- For MFC controls/layout/lifecycle changes, build and run `NativeUiTests.vcxproj` in Debug and Release and smoke-test the application.
- Use `NativeUiTests.exe --zoom` for wheel routing, per-tab scale, font lifetime, edit retention and zoom geometry checks; include `--async-save` when changing view operations allowed during Save.
- Use `NativeUiTests.exe --workspace` for file retention, path browsing, source snapshots and close/save routing. Startup/forwarding changes also require `--workspace-process <formal-exe>` with no unrelated normal instance running; this launches byte-matched isolated formal EXEs, including a renamed copy.
- After clean-script changes, run the shared clean scan and preservation verifier with `-IntermediateRoot 'artifacts\obj' -CheckNestedDelivery -CheckReparseRejection`; Codex owns the actual-project preservation check.
