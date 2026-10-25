# Native workflow acceptance - 2026-10-25

All nine approved native first versions N01-N09 are complete and accepted.
Product instructions: [Native workflows](../NATIVE_WORKFLOWS.md). Exact delivery
identity, assertion counts and changed source inputs:
[native-workflows-20261025.json](native-workflows-20261025.json).
Publication remains deferred.

## Changed files and behavior

Paths in this table are relative to `src/`.

| Row | Components | Behavior and reason |
| --- | --- | --- |
| N01 | `core/folder_text_rules.*`, `core/folder_sync.*`, `ui/advanced_workflows.cpp`, `ui/advanced_compare.cpp`, `ui/advanced_folder_view.cpp` | Independently evaluate explicit text rules on selected folder-file extensions, retaining raw synchronization states and showing evaluation failures. |
| N02 | `core/binary_workflows.*`, `hex_view.*`, `hex_view_edit.cpp`, `hex_view_decoding.cpp` | Read-only UTF-8/UTF-16/Big5 decoding beside bytes; map complete characters across rows to physical byte selections. |
| N03 | `ui/code_editor.cpp`, `main_frame_editor.cpp`, `core/binary_workflows.*`, `main_frame_workflows.cpp`, `main_frame_edit.cpp`, `main_frame_save.cpp`, `hex_view.cpp` | Separate text bookmarks from native change-history markers; review saved-baseline Hex ranges and expire stale highlights. |
| N04 | `ui/command_palette.*`, `main_frame_workflows.cpp`, `workspace_navigation.cpp` | Search actual menu commands and invoke their native handlers with context/busy guards; refuse dispatch after the originating tab changes. |
| N05 | `core/binary_workflows.*`, `main_frame_workflows.cpp` | Stream ASCII/UTF-16 string ranges with bounded prefix previews and protected CSV export. |
| N06 | `core/binary_workflows.*`, `main_frame_workflows.cpp` | Selected unsigned fixed-width arithmetic/bit operations with explicit endian/modulo semantics and one Undo transaction. |
| N07 | `core/workflow_views.*`, `ui/advanced_compare.*`, `ui/advanced_compare_actions.cpp`, `ui/advanced_compare_draw.cpp` | Collapse/expand unchanged comparison context using original source mappings; preserve merge output, Undo and reports. |
| N08 | `workspace_navigation.cpp`, `workspace_frame.*`, `main_frame_clone.cpp`, `main_frame_zoom.cpp` | Bounded, generation-guarded Back/Forward across file Text/Hex/shared-view positions; skip closed or stale locations. |
| N09 | `core/workflow_views.*`, `ui/advanced_compare.*`, `ui/advanced_folder_view.cpp`, `ui/advanced_workflows.cpp` | Optional folder-result tree with ancestors and subtree status/counts; preserve the full filtered Sync-all candidate set. |

Shared integration: `main_frame.h`, `main_frame.cpp`, `main_frame_binary.cpp`,
`main_frame_compare.cpp`, `workspace_sessions.cpp`, `core/workspace_session.*`,
`services/editor_job.h`, `resource.h`, `simple_editor_tool.rc`, and the product,
core-test and UI-test project registrations. Optional version-1 workspace fields
retain older-file compatibility. No library or runtime dependency was added.

Tests: `tests/native_workflow_tests.cpp`, `tests/native_workflow_ui.inc` and the
existing core/UI dispatchers. Documentation: README, SPEC, local AGENTS,
`NATIVE_WORKFLOWS.md`, active implementation ledger, HANDOFF and SELF_CHECK.

## Verification approach

Build core tests in Release, run `--workflows` and affected folder, merge,
file-state and workspace-parser cases. Build native UI tests in Debug and Release
and run `--workflows` with captures, plus the affected code-editor, search,
shared-view/discovery, workspace, advanced-compare, async-save, zoom, recovery and
folder-tools filters. The selected regressions follow the shared editor state,
marker allocation, optional workspace fields, presentation mappings and worker
adoption/locking changes. Unchanged firmware/format/structured algorithms reuse
their prior evidence; no blanket full-suite rerun is claimed.

After final product builds, test byte-matched formal EXE copies with
`--workspace-process <formal-exe>` and `scripts/verify_cli.ps1 -Executable ...`.
Inspect PE dependencies/security flags, delivery contents and original INI hashes;
run the workspace clean static scan and actual clean-preservation verifier.

## Actual results

Core Release assertions: workflows **37**, merge-sync **47**, folder-tools **31**,
file-safety **28**, recovery **11**, discovery **20**. Native UI assertions in
each of Debug and Release: workflows **43** (including six capture checks),
code-editor **26**, search-tools **32**, discovery **19**, discovery-completion
**3**, workspace **35**, advanced **52**, async-save **32**, zoom **34**, recovery
**13**, folder-tools **12**. All passed. The final presentation-only correction
reran workflows/advanced in both configurations and core workflows; unchanged
affected regression results from this implementation run remain applicable.

Cases cover real native palette dispatch and a tab switch during its modal
dialog, disabled entries, shared history/bookmark markers, preserved Undo,
cross-row decoded selection/read-only typing and glyph overhang, one-Undo
numeric edits without disk writes, saved-baseline range invalidation, actual
string extraction/CSV/source protection, cross-session navigation, independent
folder raw/rule filters and visible columns, retained collapsed Sync-all scope,
deep ancestor aggregation, context placeholders and workspace persistence.
Core cases additionally cover cancellation retaining old outputs, chunk/UTF-16
phase/surrogate boundaries, unsigned operations/invalid widths and source quotas.

Review corrections were verified for the empty comparison-progress callback,
the old clear-bookmark marker number, bounded surrogate preview prefix,
cross-row decoded hit testing, palette origin-tab guard, folder rule-column
refresh and deep-tree aggregation order. Final review found no remaining high
or medium risk issue in this scope.

Canonical `scripts/build.ps1 -Configuration Debug` and
`scripts/build_release.bat` passed without reported compiler/linker warnings or
errors. Exact target preflight found no target instance requiring closure;
automatic closure is not claimed as exercised. The final Release EXE is
**3,384,320 bytes**, SHA-256
`B4D36D863CF3F9165F773D9B0CC8DE561770BF4B19ED8017300458C4B96B10EF`.
The prior scope's baseline was 3,271,680 bytes; this is a feature addition,
with no EXE size optimization claim.

Byte-matched formal Debug and Release copies each passed **12 GUI process
assertions** and **7 CLI cases**, including maximized startup, single-instance
forwarding, retained multi-file tabs, selected close, shutdown/relaunch, report
exit statuses and output/source protection. Isolated GUI INI files were retained.
Release PE inspection confirms x64, ASLR/high-entropy/NX and only Windows system
imports. MFC/CRT/Scintilla remain statically linked; no new runtime, package or
driver installation requirement was introduced.

Clean audit: **99** scripts passed the static known-pattern scan; **17** fixture
files and source were preserved, with idempotence and junction rejection.
Actual-project clean removed **699** allowlisted compiler intermediates and
preserved **18** protected actual files by SHA-256. `build/` contains exactly
the EXE and the original three INIs, whose original hashes remain unchanged.
Six actual native captures per configuration were inspected. Source/project
freeze: **224** inputs in `artifacts/evidence/native-workflows-final-inputs.json`.
Raw evidence stays local in `artifacts/logs/workflow-*` and
`artifacts/screenshots/workflows-{debug,release}`. The next build regenerates
cleaned compiler objects. No remote, commit or upload was created.

## Operational limits

- Folder text inputs: 16 MiB per file; explicit extension/encoding/literal rules.
  Semantic equality never silently suppresses a raw synchronization operation.
- Visible Hex decoding: 64 KiB plus bounded lookbehind. Ambiguous Big5 boundaries
  display dots until a proven boundary. Decoding is a read-only column.
- Modification lists and string previews retain at most 100,000 rows. String
  previews retain at most 128 UTF-16 units; CSV exports these previews and full
  physical ranges, not unlimited string contents.
- Numeric selections are at most 64 MiB and divisible by the selected word size;
  unsigned arithmetic is modulo 2^width and shift counts must be below width.
- Context count is 1-50, with 0 disabling collapse. Navigation retains 256
  in-memory locations for file editors; comparison-source rows are excluded.
- Result expansion state is temporary. Tree collapse preserves synchronization
  candidates and the existing operation preview/revalidation/backups.
- Native synthetic-window/process and local-file tests do not establish clean-PC,
  elevated drag/drop, network-drive, all-monitor/DPI or arbitrary-language parity.
  These bounded additions do not claim complete competitor parity.
