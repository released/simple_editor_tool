# Alignment acceptance, 2026-10-25

The 30 recommendations in FEATURE_ALIGNMENT_REVIEW.md have implementation or
evaluation dispositions in ALIGNMENT_IMPLEMENTATION.md. X04 requested evaluation
of native recording/batching and plugins; its decision is documented, not a
macro-recorder implementation. X02/X03 include both implemented native formats /
templates and assessments of independent extensions. This is not full feature
parity with every referenced application.

## Changed components and purpose

| Files / area | Change and reason |
| --- | --- |
| `src/hex_view*`, `src/main_frame_binary.cpp`, `core/binary_report.*` | Range/ASCII/clipboard editing and full streamed address reports |
| `core/binary_analysis.*`, `binary_merge.*`, `firmware_image.*`, `compare_region.*`, `pattern_search.*`, `main_frame_tools/regions/merge/formats.cpp` | Numeric/hash/entropy, range/firmware tools, regional comparisons and reviewed merges |
| `NativeEditor.vcxproj`, `vendor`, `ui/code_editor.*`, `editor_drop_target.cpp`, `native_search.*`, `search_results.*` | Static native editor, highlighting/navigation, Unicode bridge, real file-drop handling and advanced search |
| `core/line_merge.*`, `diff_presentation.*`, `text_report.*`, `text_output.*`, `ui/advanced_compare*`, `diff_overview.*`, `main_frame_patch/search.cpp` | Intraline/moved/similar display, coherent manual/block Undo, output searches and guarded reports/patches |
| `core/folder_sync.*`, `folder_policy.*`, `sync_journal.*`, `ui/advanced_folder_view.cpp`, `sync_preview.*` | Criteria, explicit directional/baseline policies, per-operation preview and guarded retry/recovery |
| `workspace_documents/preferences/recovery/search.cpp`, `core/temporary_documents.*`, `portable_preferences.*`, `recovery_store.*`, `services/multi_file_search.*`, `ui/preferences_dialog.*`, `multi_search_dialog.*`, `workspace_tabs.*` | Retained untitled tabs, preferences, opt-in isolated draft recovery and incremental multi-file searches |
| `app.cpp`, `core/command_line.*`, `services/cli_console.*`, `scripts/verify_cli.ps1` | Headless read-only comparisons/reports and real EXE exit/output acceptance |
| `core/structured_compare.*`, `ui/advanced_structured.cpp` | CSV/TSV logical cells and exact normalized JSON comparisons |
| `core/binary_template.*`, `ui/template_dialog.*`, `main_frame_template.cpp` | Read-only bounded structure templates and native field navigation |
| `core/text_page.*`, `ui/text_pager.*`, `main_frame_pager.cpp`, `services/difference_mailbox.h`, `main_frame_compare.cpp` | Bounded encoding-safe pages and incremental binary rows, with cancellation/worker cleanup |
| Main/workspace headers, resources, project files and `core/workspace_session.*` | Integrate actual commands, session settings and static compilation |
| `tests/native_*`, product guides, README, SPEC, ledger, SELF_CHECK/HANDOFF, `.gitignore` | Focused acceptance, accurate public entry/guides and separation of private generated evidence |

## Verification and reproduction

Use `scripts/build_release.bat` and `scripts/build.ps1 -Configuration Debug`.
Both canonical builds passed. Native UI fixtures were built in Debug and Release
using NativeUiTests.vcxproj. NativeCoreTests.vcxproj supplies the affected core
modes; the ledger retains earlier applicable evidence without claiming a new
full-suite run.

- Core CLI 14, structured 14, templates 10, paging/stress 11 passed. The report
  API changes also passed binary-tools 12, text-reports 22 and merge-tools 19.
- Debug/Release native structured 6 and progressive-compare 4 passed. Latest
  inspection passed Debug 11 / Release 13 including two actual dialog captures;
  monitor work-area fitting, resize/retained buffers and footer visibility passed.
  Captures of the field tree, paged view and dark untitled workspace were visually
  inspected. Connected regions 6 and Release workspace 35 also passed.
- Exact Debug and Release formal EXE copies passed 12 GUI process checks each:
  concurrent/renamed single-instance forwarding, retained file tabs, ready-window
  maximization, selected close, normal shutdown and singleton release. CLI passed
  seven process cases per configuration, with spaced paths, exit codes, report
  overwrite protection and unchanged isolated GUI INI.
- Workspace clean scan passed for 93 scripts. Clean preservation passed 17
  protected fixture files, idempotence and junction refusal. Actual project clean
  removed 569 allowlisted intermediates while 18 protected files retained their
  SHA-256 values. All three original delivery INIs retained their original hashes.
- README/guides' local links, product-only README sections and private path
  screening passed. The runtime tree contains no Python sources. Publication
  remains deferred; no GitHub remote/upload was created.

The exact artifact, import inventory, original INI hashes and measurements are in
[alignment-20261025.json](alignment-20261025.json). Final EXE: 3,035,136 bytes,
SHA-256 `D216B7CE90D07909DEACE93F049ACF914FCA76FA00E08C11C189CFC82385A179`.
The complete four-file delivery is 3,035,859 bytes. x64 PE, ASLR/high-entropy ASLR
and NX passed inspection. MFC/CRT/editor are statically linked; imported and
native dynamically loaded graphics/control libraries are Windows components.
No new installation or driver is required. Cleaning preserved that exact EXE.

## Measured limits and gaps

Three sequential runs without a concurrent build used two owned 1 GiB sparse
files. First/last text pages took 2, 4, 2 ms; complete equal-address binary
comparisons took 650, 638, 656 ms and found the final-byte difference. Progress
cancellation took 11, 9, 9 ms. Core process peak working sets were 7,823,360,
7,819,264 and 7,823,360 bytes. Only written end regions were decoded as text;
sparse holes contain NUL and are not valid text. These are synthetic core
measurements, not whole-MFC-memory figures, paired optimization results, a file
size maximum or general storage throughput promises.

Native OS clipboard transport was NOT RUN: Windows refused the isolated station
with error 5; fixtures did not change the user's clipboard. Physical drag/wheel,
multiple physical monitors and real remote/device sources were NOT RUN. Message
and isolated-store tests are labeled separately. Windows-managed DPI scaling
remains; native per-monitor transitions are not implemented. Additional extension
decisions and absent macro/plugin compatibility are explicit in
EXTENSION_ASSESSMENT.md. These boundaries remain visible in product guides.

Initial failed fixture evidence is retained locally: an early maximization check
ran before startup reached its ShowWindow call; it now waits for the ready state.
The pager layout's `int`/`LONG` template mismatch was corrected before the final
successful builds. No failed result is represented as a passing build.
