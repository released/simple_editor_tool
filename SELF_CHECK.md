# Self Check

## Completed verification: comparison notice / virtual rows (2026-10-30)

- PASS: old virtual mappings/native counts retire before snapshot adoption;
  Cell/custom draw reject late out-of-range rows. The supplied NG.jpg failure
  and prior partial-folder abort are resolved by the original regression.
- PASS: --compare-notice in Debug (36 checks with image) and Release (35), plus
  advanced, document-focus, side-merge, presentation, compare-page, localization
  in both. Canonical Release passes workspace-process (12 checks): 15 new groups.
- PASS: native Traditional Chinese popup image inspected. Cancel/failure/partial/
  truncated/filtered results cannot claim equality. Copy/Undo auto compare stays
  quiet; source drafts, independent output and guarded saves remain intact.
- PASS: x64/static MFC/CRT, Windows-system DLL imports, high-entropy ASLR/NX.
  Core localization result from the prior checkpoint is reused explicitly.
- Final TST-01/TST-02 and preservation measurements are recorded in
  docs/verification/compare-notice-20261030.json. Older checkpoint entries below
  describe the earlier incomplete state and do not override this verification.
- TST-01 PARTIAL: removed 674 compiler files / eight reviewed outputs; all 714
  protected content hashes unchanged. No current-batch fixture/test EXE remains.
  All 67 prior remnants / 832,118 bytes still require cleanup: the additional
  15 obsolete fixture files were hash/owner verified but rejected before execution
  by automatic safety review. No alternate deletion or retention exception.
  TST-02 PASS: complete project remains below 20,000,000 bytes, including NG.jpg.

## Shutdown checkpoint: compare equality notice (2026-10-30)

- PASS: source Debug UI build and core localization (20 checks plus footer).
- INCOMPLETE: --compare-notice emitted 22 PASS checks, then exited 3 at/after
  the failed-folder case before the synthetic partial-folder case completed.
  Investigate the exact abort and native-dialog teardown before final acceptance.
- NOT RUN: final Release UI regression, canonical Release update, formal smoke
  and localized popup screenshot. Desktop computer-use pipe was unavailable.
- Preserve the previously verified canonical EXE. HANDOFF.md contains exact
  versions, failure location, retained fixture ownership and resume steps.
- Shutdown clean: 594 compiler files and nine reviewed test/build outputs removed;
  no test EXE remains. Canonical EXE/runtime INI retained unchanged. Complete folder
  about 19.49 MB. Cleanup remains incomplete for prior 49 files and 18 interrupted/
  failed fixture files (133 bytes); exact provenance and the rejected first-run
  retirement are recorded in compare-notice-20261030-checkpoint.json.

## Two-way source copying and central line list (2026-10-28)

- REQ-01 PASS: middle original-line list, main-height sources, optional retained
  output, selected/whole L-to-R and R-to-L copies, next/previous block buttons.
- PASS: --side-merge in Debug and Release checks actual HWND geometry, source
  selection, separate differences under ignored EOL, inactive target creation,
  draft-only copies, native Undo/Redo, stale/read-only rejection, strict Big5
  representability, destination EOL, side Save/Ctrl+S, resize and zoom.
- PASS: --compare-page, --advanced, --workspace, --pasted-text, --zoom,
  --localization, --presentation, --editor-review, --workflow-expansion,
  --workflows and --async-save in both configurations. Both formal EXEs pass
  --workspace-process. Core Release: --merge-sync, --presentation, --merge-tools,
  --localization, --text-reports and --document-focus. 32 selected groups pass.
- PASS: canonical Release is x64, statically linked MFC/CRT, Windows-system
  imports only, ASLR/high-entropy/NX; source/delivery language catalogs match.
  One native image inspected. Remaining manual desktop/DPI coverage and final
  TST-01/TST-02 measurements are recorded in the versioned verification JSON.
- TST-01 PARTIAL: 946 compiler and 12 reviewed test/build files removed; 709
  protected hashes identical. No test EXE/current fixture remains. Historical
  47-file typography archive and two regenerated Debug INIs (49 / 831,985 bytes)
  were hash/owner verified, but exact retirement was rejected before execution
  by automatic safety review. No alternate deletion or retention exception;
  full cleanup/delivery remains incomplete. TST-02: under 20,000,000 bytes.

## Comparison page behavior (2026-10-28)

- PASS: --compare-page Debug 31 / Release 32 assertions including one final
  screenshot. Covers closed-file history/persistence, exact editable paths,
  default text/explicit binary modes, side-specific OLE/legacy/root drops,
  invalid/busy rejection, source/merge/Undo retention, actual z-order and first
  native repaint. Inactive file Base is excluded from folder history. Lazy
  function-sidebar and shared-Text windows retain the correct z-order after layout.
- PASS: affected workspace, drop-targets, advanced, pasted-text, zoom, async-save,
  localization, presentation and outline/sidebar callers in both configurations.
  The final shared-layout audit reran every selected UI mode on final binaries.
  Formal Debug/Release process tests add twelve assertions each:
  twenty-two selected passing runs / 669 assertions.
- PASS: canonical Release, native x64/static runtime/Windows-only imports/security
  flags, paired guides and exact source/input hashes. Existing C4458 warnings are
  recorded. Test settings are synthetic; real app closure changed recent keys only.
- TST-01 this batch PASS: safe clean removed 846 compiler files; reviewed purge
  retired ten final test/build files (111,474,395 bytes); 705 protected hashes matched.
  No new test-root files or test-only EXEs remain. Prior 49 files / 826,431 bytes
  remain blocked and unchanged, with retention rejected; global cleanup/delivery
  is still incomplete. No new attempt to delete that blocked set or new exception.
- TST-02 PASS: complete project below 20 MB after previously authorized lossless
  Git repack; all 859 objects/four references/reflogs and full fsck pass. All 672
  non-Git protected hashes match. Evidence and exact bytes:
  docs/verification/compare-page-20261028.json. Physical Explorer/mixed-DPI and
  fault-injection checks NOT RUN; unrelated core algorithms were outside the change.

## Native UI typography (2026-10-27)

- PASS: Debug/Release --outline-sidebar (52/55 assertions), --zoom (51 each),
  --localization (17 each), --workspace (35 each); formal Debug/Release products
  build and pass --workspace-process (12 each). Three Release native renders;
  final Chinese Text/sidebar/notice and English Compare images inspected.
- PASS: explicit normal sans-serif fonts, native caption bounds, language/tab
  switching, source/Undo/zoom retention and owned font lifetime. PE remains native
  x64/static runtime with Windows-only imports and existing security flags.
  Existing C4458 warnings in two unchanged notes files are recorded, not hidden.
- PASS: four-script clean scan, 842 compiler files and ten test/build outputs
  retired, 1,527 protected hashes matched. User-approved lossless Git repack
  preserves all 832 objects/references/reflogs; full fsck and 667 non-Git protected
  hashes passed. Only the formal Release EXE remains in the entire project.
- TST-01 BLOCKED: user rejected temporary retention of the final 49 owned files /
  826,431 bytes. The fixed-name/SHA-256 manifest was also rejected by automatic
  review before execution, without a detailed reason. All 700 current protected
  hashes match; all 49 targets remain. No alternate tool/move/truncation or
  retention exception; delivery remains incomplete. TST-02 PASS: complete project
  remains below 20 MB, including Git and the blocked local-only/ignored scratch.
- Evidence, remaining paths, source/build hashes and exact bytes:
  docs/verification/ui-typography-20261027.json. Unchanged algorithm/CLI and
  boundary-fixture evidence reused; physical input/mixed DPI/resource injection
  NOT RUN. Existing settings retained; normal close changed recent-file keys only.

## Automatic function sidebar (2026-10-27)

- PASS Debug/Release: core --outline-sidebar, --document-focus, --localization;
  native --outline-sidebar, --code-editor, --text-workflow, --workspace,
  --localization, --zoom, --async-save and --document-focus. Coverage includes
  language routing, prototypes/MFC maps, shared-view edits/Undo, stale result
  rejection, original-line click/Enter navigation, tab widths, Save As and close.
- PASS: canonical Release; formal --workspace-process (12 assertions), seven
  CLI exits; native x64/high-entropy ASLR/NX, static runtime and Windows-only DLLs.
- PASS: both final native sidebar images inspected; strict UTF-8/local guide
  links; 730 paired stable catalog keys, matching delivery copies and original
  runtime INI hashes. README remains product-facing.
- PASS: four-script static clean scan. Unchanged cleanup boundary/fixture
  evidence remains applicable. Actual safe clean removed 850 compiler files;
  reviewed purge retired eleven test/MSBuild outputs. All 1,475 protected hashes
  matched twice. Retired 74 fixed-list expected-hash files / 4,384,863 bytes.
  TST-01/TST-02 PASS: no artifacts or disposable test EXEs remain; full project
  is about 18.52 MB, below 20,000,000 bytes, with no external cleanup or exception.
  Evidence: docs/verification/outline-sidebar-20261027.json.
- NOT RUN: physical input, mixed DPI, maximum-quota throughput and injected
  resource/power failure. Supported heuristic subsets are explicit in the guides.

## Document-focused F01-F09 (2026-10-26)

- PASS: Debug/Release core --document-focus (39 assertions), --localization,
  --preferences, --merge-sync, --search-codec and --workflow-expansion.
- PASS: Debug/Release native --document-focus (42 assertions, plus two Release
  captures), --localization, --preferences, --advanced, --workflows,
  --workflow-expansion, --editor-review, --text-workflow, --zoom, --folder-tools
  and --text-reports. Final Report state guard reran the three affected modes in
  both configurations; other still-applicable caller runs retain their original
  binary hashes. Details and commands: docs/verification/document-focus-20261026.json.
- PASS: canonical Release, formal --workspace-process (12), seven CLI exit cases,
  native x64/high-entropy ASLR/NX and Windows-only imports. The three original
  runtime INIs retain entry hashes; source/delivery catalogs match (719 pairs).
- PASS: strict UTF-8, reciprocal/local links and 18 topics per full guide;
  product-only README semantics; three final native Release images inspected.
- PASS: four-script static clean scan. Reuse unchanged boundary/fixture checks
  from project-cleanup-boundary-20261026.json. Actual clean removed 837 compiler
  files; reviewed purge retired ten test/MSBuild outputs. Both checkpoints matched
  all 1,266 protected hashes. Retired 149 owned files / 4,400,984 bytes using a
  fixed expected-hash list and project/run boundaries. TST-01/TST-02 PASS: no
  artifacts or test-only EXEs remain; complete folder is about 15.31 MB, within
  20,000,000 bytes. No external deletion or retained scratch exception.
- NOT RUN: physical clipboard/wheel, mixed DPI, maximum-quota throughput and
  injected resource/power failure. Product subsets are explicit in the guides.

## English / Traditional Chinese and full user guides (2026-10-26)

- PASS Debug/Release: core --localization (20 checks), --preferences (9); native
  --localization (17), --preferences, --text-workflow, --workspace, --advanced,
  --editor-review, --workflow-expansion, --zoom and --async-save. Counts exclude
  harness footers. Real language switching retains user buffers/selection/Undo,
  search input, source protection, output and jobs; newest preference wins.
- PASS: canonical Release; byte-matched formal first-run Traditional Chinese,
  persisted English, immediate switch/restart, missing and invalid catalog English
  fallback (24); actual formal single-instance/workspace process (12); CLI seven
  exit contracts; native x64/high-entropy ASLR/NX and Windows-only imports.
- PASS: both complete 17-topic UTF-8 guides, reciprocal/README links, product-only
  entry/usage content, paired catalog stable keys, delivered catalog hashes and
  original INI preservation. Two Release Chinese screenshots visually inspected.
- PASS: four-script shared static clean scan. Unchanged safe-clean fixture and
  boundary-rejection evidence reused from project-cleanup-boundary-20261026.json;
  final actual clean/purge/hash preservation and size are recorded in
  docs/verification/localization-20261026.json. No external cleanup.
- TST-01/TST-02 PASS: 777 compiler files, 10 test/metadata outputs and all 68
  owned scratch files retired. All 1,238 protected hashes matched after actual
  clean/purge and before scoped scratch retirement. artifacts is absent, original
  INIs remain unchanged and complete folder is below 20,000,000 bytes.
- NOT RUN: physical clipboard/mouse, mixed DPI, exhaustive dialog translation,
  injected allocation/power failures and maximum-quota throughput. Unlisted
  diagnostics retain English; native Windows dialogs follow OS language.

## Workflow expansion W01-W10 (2026-10-26)

- PASS Debug/Release: source gaps/original coordinates, inline source indicators,
  Base visibility/output independence, preset save/apply/delete, encoding/BOM/EOL
  selection mapping and stale guards, Unicode live/allowlist/detail/Undo,
  external keep/compare/cancel/reload, entropy mouse selection/version rejection,
  conversion preview/invalidation/readonly/one Undo and JSON native rules.
- PASS: four relevant core and nine UI modes per configuration. Dialog geometry
  follow-up reran expansion mode in both; three final native Release images were
  inspected. Exact formal Release process and seven CLI cases PASS.
- PASS: final PE is native x64 with ASLR/high-entropy/NX and system-only imports.
  test.ini and ui_smoke.ini retain entry hashes. A newer main runtime INI is
  retained explicitly; its difference from the entry record is not an unchanged-
  INI PASS. README remains product-facing.
- NOT RUN: physical clipboard interoperability, maximum-quota throughput,
  physical wheel gestures, mixed-monitor DPI and injected resource/power failures.
  Subset/precision limits are explicit in docs/WORKFLOW_EXPANSION.md.
- TST-01: verified safe compiler clean removed 765 files; reviewed purge removed
  11 test-only executables/metadata files. All 1,218 protected files matched the
  cleanup-boundary checkpoint. Unchanged safe-clean fixture evidence was reused.
  TST-02 PASS: all 120 owned scratch files (1,581,575 bytes) retired, artifacts
  absent, complete folder below 20,000,000 bytes including hidden/Git/vendor data.
  Exact bytes: docs/verification/workflow-expansion-20261026.json; no external deletion.

## Editor review V01-V10 (2026-10-26)

- PASS in Debug and Release: all approved review flows, native UI lifecycle,
  chosen-source Find, readonly source Undo, draft/merge preservation, all-open
  selected preview/cancel/stale guards, current/all-open multiline replacement,
  mark coexistence/invalidation, outline tree jump, indentation/manual/clone rules,
  moved pairs, bitfield menu/edit/readonly/stale guards and IPS preview/import/export.
- Five core and seven UI modes per configuration PASS; new modes have 38 core
  and 59 UI checks. Canonical Release, formal process (12 checks), CLI seven-case
  exit contract, native x64/ASLR/high-entropy/NX and system-only imports PASS.
- Both retained Release images inspected. Three original INIs retain entry hashes.
  Product instructions are separate from engineering records. Final source/build,
  cleanup preservation and complete-folder bytes: docs/verification/editor-review-20261026.json.
- Limits: explicit EditorConfig indentation subset; equal-length IPS/IPS32 only;
  exact unique complete unmatched moved blocks. Full quota stress/throughput,
  physical clipboard/mouse/mixed DPI, injected allocation/power failures and broad
  third-party patch interoperability NOT RUN. No new runtime, driver, commit/push.

## Text defaults / Markdown / scoped Find/Replace (2026-10-26)

- PASS: new text defaults to Text; binary fallback, existing-session page/draft
  retention, #3F3F3F content, Markdown style recognition/colors/decorations and
  language/theme changes that preserve generation, text and Undo.
- PASS: Ctrl+F replacement controls and scope selector; all-open unsaved/untitled
  Count/results/navigation, both directions/wrap/F3, single/all replacement,
  cloned-view locks/updates, per-document Undo, unchanged disk files, read-only,
  cancellation, version preflight, missing/closing-session guards, escaped Unicode,
  regex/error handling, unsupported Hex scope and current-only replacement.
- PASS in Debug and Release: native --text-workflow (48), --workspace (35), --zoom
  (51); connected --search-tools (32), --preferences (14), --cross-search (18),
  --code-editor (26). Counts exclude the harness footer. Core --search-codec and
  --preferences PASS per configuration. Reused connected tests precede only the
  final missing-session guard and explicit Hex setup in the zoom fixture; affected
  workflow/workspace/zoom paths were subsequently rebuilt and rerun in both modes.
- PASS: canonical Release build, formal --workspace-process (12), seven CLI cases
  with expected exits 0/0/1/1/2/1/2, x64/ASLR/high-entropy/NX and system-only imports.
- PASS: native Release Markdown and Find/Replace captures visually inspected.
  Full 128 MiB aggregate stress/performance testing was not run; bounds were
  reviewed, and normal/Unicode draft, cancel, lock and stale paths were exercised.
- Final owned cleanup: PASS; owned artifacts removed, project below 20 MB. The temporary 512 MB budget was exceeded
  during stock Release linking (observed 639,110,240 bytes); notified and immediately
  removed compiler output. Later bounded UI builds stayed below 200 MB. Final
  delivery/config/source hashes, artifact removal and full-folder byte measurement
  are recorded in docs/verification/text-workflow-20261026.json.


## Pasted-text comparison and cleanup complete (2026-10-26)

- PASS: native memory-only input panes, separate pasted comparison sessions,
  background snapshots/locks, Unicode/mixed endings, empty sides, 60,000-line inputs,
  Undo/Redo, swaps, zoom, stale-result clearing, cancellation, quotas/NUL rejection,
  output save, close cancellation, workspace omission and obsolete preview release.
- Final Debug/Release each PASS: --pasted-text 36 and --workspace 35, exit 0.
  Initial --advanced 52 and --zoom 51 per configuration remain applicable to
  unchanged paths after the mode-8-only preview follow-up. Native screenshot
  inspected at 150%: both inputs, inline difference markers and output visible.
- Canonical final Release build PASS; 3,404,288 bytes, SHA-256
  30A37C1D37DFFA694A6F6B89E2622BDEF974E2E7AC6B9A355249278B5611E2CF.
  Formal CLI seven expected exit-code cases PASS; copies/fixtures cleaned.
  PE x64/high-entropy ASLR/NX and Windows-only imports PASS; no new installation.
  Existing three INIs retain baseline hashes. build holds only EXE/INI files.
- Cleanup PASS for compiler intermediates via verified clean and completed target
  EXEs via their MSBuild Clean targets. Native test processes exited normally;
  no synthetic set*/cli-owned fixtures remain. No project-external deletion.
- TST-01/TST-02 PASS: user rejected retention exceptions; project is below
  20,000,000 bytes. Independent fixed-path/owner/hash-checked file operations
  completed after earlier bulk review rejections. Removed all 111 current-run
  files / 39,346,361 bytes, including 44 large spills / 38,764,503 bytes.
  Removed 42 older audit fixtures / 58,173 bytes and detached only their verified
  project-local junction entry without recursion. Existing purge preview/Apply
  removed 11 recognized compiler metadata files. artifacts and empty roots are gone.
- Preservation PASS: all 544 protected hashes matched after large spill removal;
  final EXE, three INIs, 15 compiled-source hashes and public screenshot match.
  Only verification/handoff records were intentionally updated afterward. Concise
  JSON/PNG evidence retained; no raw archive, test binary or project-external deletion.
  No application build/test rerun for this data/document-only cleanup follow-up.
- Temporary 512 MB exception was approved. Initial stock Debug accumulation had
  a 566,585,019-byte observed sample (actual peak not captured); owned compilation
  was stopped and verified clean removed 281 intermediates. Later UI builds omit
  symbols/incremental cache, retaining Debug runtime/assertions, and batches clean
  separately. Product settings are canonical; final CL=/MP2 scheduling restored.
- Evidence/commands/source hashes/cleanup scope: docs/verification/pasted-text-20261026.json;
  native image: docs/verification/pasted-text-20261026.png. Physical clipboard/mouse,
  mixed-monitor DPI and injected allocation/power-failure cases NOT RUN. No commit/push.

## Project boundary for cleanup (2026-10-26)

- PASS: scripts/artifact_helpers.ps1 anchors cleanup to the script's project,
  validates its metadata, requires narrower caller roots and retains existing
  normalized-path/reparse checks. System, profile, global TEMP, volume, parent,
  sibling and whole-project roots cannot be substituted. The eight probes only
  validate strings; they never create or delete external files.
- PASS: tests/verify_artifact_cleanup.ps1 now creates and removes its owned fixture
  under project artifacts. All 20 assertions pass, including preview/idempotence,
  source/delivery/unknown preservation, junction preflight and CLI success/failure
  cleanup. Four affected/calling PowerShell scripts parse without errors.
- PASS: shared static audit covers 112 scripts. Normal-clean help/idempotence and
  junction refusal pass; 17 protected fixture files/source and all four actual
  build files retain their SHA-256 hashes. Shared verifier TEMP/TMP was redirected
  only for its process to a fresh project-owned directory and restored in finally.
- RETENTION EXCEPTION: automatic approval review rejected the final project-local
  audit deletion before execution, without a detailed reason; no alternative
  deletion attempted. Retained 42 files / 58,173 bytes and one internal junction
  in artifacts/scope-validation-39ac94dababb4244b98baef625dd40f2. Full artifacts purge
  refuses that junction; revisit this exact owned audit when safe permitted cleanup
  is available. No project-external deletion; earlier external data is untouched.
- Evidence: docs/verification/project-cleanup-boundary-20261026.json. PASS: shared
  and local rule/document consistency, strict UTF-8 and balanced Markdown in all
  nine edited documents, git diff --check, ignored audit and no tracked artifacts.
  Application build/unrelated suites NOT RUN: cleanup PowerShell/docs only.
  No native source/runtime/settings changes, commit or push.

## Artifact retention and cleanup (2026-10-26)

- Complete: reviewed 476 local artifact files (233,585,523 bytes / 222.76 MiB)
  and removed their identified generated output. artifacts now has zero files;
  empty directories, including its root, were removed. Compact historical
  acceptance summaries remain under docs/verification. Old records' raw artifact
  paths were deliberately pruned after acceptance and are not permanent files.
- Removed categories: nine retained cli-owned copies/data sets, generated test
  and Debug EXEs, an out-of-place 51,805,116-byte ILK, old PyInstaller packaging,
  archived layout baseline, cached upstream ZIPs, old logs/manifests/screenshots.
  Cached ZIP hashes matched pinned vendor versions; source uses vendor directly.
- Changes: /artifacts/ is ignored as a whole. scripts/purge_artifacts.ps1 defaults
  to preview and requires explicit -Apply; classification is an allowlist, with
  source/build outside its root, full reparse preflight and unknown/runtime target
  preservation. scripts/artifact_helpers.ps1 shares guarded path/fixture handling.
  Normal clean remains unchanged and conservative; it never calls purge.
- scripts/verify_cli.ps1 now cleans owned copies in finally on success/failure,
  disposes processes and provides explicit -KeepArtifacts for diagnostics.
  NativeUiTests.vcxproj sends its incremental-link database to IntDir beside PDB.
  README/SPEC/AGENTS document the current retention and cleanup commands.
- Actual safety checks: tests/verify_artifact_cleanup.ps1 12 PASS, covering preview,
  idempotence, preserved source/delivery/unknown files, path escape, full rejection
  before deleting through a junction, unowned fixture protection, failure cleanup,
  real CLI success cleanup, KeepArtifacts and retained unknown notes. Real project
  scripts/verify_cli.ps1 separately passed all seven cases and left zero files.
- Shared checks PASS: static scan 111 scripts; regular-clean fixture preserves
  17 files/source, help and idempotence; junction refusal; actual-project clean
  preserves 18 protected files by SHA-256 (0 native intermediates removed).
  Explicit purge matched its preview: 476 removed, 0 unknown files found.
  Repeating purge removed zero files. All 509 protected source/vendor/test/script/
  project/delivery hashes matched before/after the actual purge.
- Debug and Release ILK metadata was evaluated with MSBuild and resolves under
  artifacts/obj/ui-tests/<configuration>. Full native rebuild NOT RUN: application
  code/runtime was not changed; the artifact-only linker metadata was evaluated.
  Existing application behavior evidence is reused, not claimed as a fresh run.
- Production EXE remains 3,394,560 bytes, SHA-256
  63889239C8C511840C4541A91946C7ED3A479C183EEEA671AB1D0A6113BE45A2.
  Existing INIs and current Explorer target are retained. build still contains
  exactly the EXE and the user's three INIs. No artifact is tracked by Git;
  future artifact categories match the root ignore rule. No commit/push performed.
- Audit data: docs/verification/artifacts-cleanup-20261026.json. Detailed temporary
  audit logs are outside the project. Automatic approval review rejected the
  extra deletion of the shared verifier's project-external temporary fixture,
  without a detailed reason; it was retained and no alternate deletion attempted.
- Current application identity remains in the Explorer acceptance record below;
  no outstanding project-artifact cleanup item remains.

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

## Completed alignment verification (2026-10-25)

Actual final artifact and all original recommendation dispositions:
docs/verification/ALIGNMENT_ACCEPTANCE.md and alignment-20261025.json.
Verification was selected by impact; the following includes retained applicable
prior evidence and explicitly identified reruns, not a new complete suite run.

- PASS: canonical Debug/Release native MFC builds. Final Release EXE 3,035,136
  bytes, SHA-256 D216B7CE90D07909DEACE93F049ACF914FCA76FA00E08C11C189CFC82385A179.
  x64, ASLR/high-entropy ASLR/NX, static MFC/CRT and Windows dependency inspection.
- PASS: exact formal Debug and Release copies each passed 12 GUI process checks
  (ready maximization, concurrent renamed singleton, retained multi-file forwarding,
  selected close, normal shutdown/relaunch) and seven CLI process cases. Isolated
  INI remained unchanged under headless invocation; report overwrite guards passed.
- PASS: final core CLI 14, structured 14, templates 10, paging/stress 11; report API
  callers reran binary-tools 12, text-reports 22 and merge-tools 19 successfully.
  Debug/Release structured 6 and progressive-compare 4 passed. Latest inspection
  Debug 11 / Release 13 includes work-area fit, resize/footer/buffer retention and
  actual screenshots. Field tree, pager and dark untitled captures were inspected.
  Connected final regions 6 and Release workspace 35 passed.
- PASS retained: core analysis 24, firmware 20, patterns 11, presentation 12,
  regions 14 plus default diff assertions, folder-tools 31, preferences 9 and
  recovery 11. Debug/Release code-editor 26, search-tools 32, format-tools 11,
  presentation 8 (Release screenshot 9), text-reports 13, advanced 52, zoom 34,
  OLE drop 10, folder-tools 12, cross-search 18, documents 14, recovery 13,
  preferences 14 (Release capture 15), affected async-save/workspace 35.
- PASS: three sequential quiet 1 GiB sparse core runs, first/last page pairs
  2/4/2 ms, exact-last-byte full compares 650/638/656 ms, progress cancellation
  11/9/9 ms; peak core working sets 7,823,360/7,819,264/7,823,360 bytes. Not whole
  GUI memory or general file/storage maximum; only written ends decoded as text.
- PASS: README product-only semantic/privacy review and product-guide local links.
  No Python source in runtime/native vendor tree. GitHub publication deferred.
- PASS: workspace clean static scan (93 scripts), preservation fixture 17 files,
  idempotence/junction refusal; actual clean removed 569 allowlisted intermediates,
  preserved 18 protected files by SHA-256. build has exactly EXE + original 3 INIs,
  all original INI hashes unchanged. No PDB/development directories in build.
- NOT RUN: OS clipboard transport (isolated station denied, Win32 error 5), physical
  drag/wheel/multiple monitors, real remote servers/process/raw devices. Synthetic
  message/isolated-store tests do not imply these. Native per-monitor DPI is not
  implemented; Windows scaling retained. Independent macro/plugin/archive/etc.
  decisions are documented in EXTENSION_ASSESSMENT.md; no false parity claim.
- No required implementation or delivery check remains within the original
  recommendation scope. Older records below are historical artifacts, including
  earlier sizes/hashes, and do not replace this final identity.

## Verified x64 EXE slimming 2026-10-25

- PASS comparable Release baseline 3151872 bytes (original 3151872);
  formal 1083904, SHA 13CE079CEFB03C01EDF826B45A74FDCAAE38E9609604885AC44807F5CB67DEA9.
- PASS scripts/build.ps1 -Configuration Release / Debug. No compiler/linker warnings/errors reported.
- PASS hash-matched isolated app startup/control counts, assertion-free Debug and
  normal exit 0. Separate Debug/Release NativeUiTests --workspace (35), --zoom (34), and --workspace-process against each exact formal EXE (11) PASS.
- Independent synthetic documents/workspaces only; original editor INIs/drafts untouched.
- PASS x64-only unused MFC registration exclusion; static runtime, existing
  optimization/security and PE 0x8160 retained, VisualManager RTTI absent.
  All application source hashes match the starting manifest.
- PASS 5 alternating AB/BA pairs after warmups and baseline-only calibration,
  without concurrent compilation. Delta max(floor, 10% median, 3*MAD); memory5%;
  floors startup25/UI2/CPU16ms, memory1MiB.
  - StartupMs: 141.1988 -> 138.4240; limit 176.1988.
  - PeakBytes: 17797120.0000 -> 17281024.0000; limit 18845696.0000.
  - CpuMs: 93.7500 -> 109.3750; limit 140.7500.
  - WorkflowMs: 2.6847 -> 2.1611; limit 4.6847.
- PASS actual scripts/clean.bat (verifier profile: artifacts/obj): 186 compiler files removed,
  all 15 protected actual files SHA unchanged. Root INI hashes match the
  pre-build manifest. 17 nested fixture files/source, help/idempotence PASS;
  prior unchanged junction checks reused, not rerun.
- Raw samples/limits, fixture scope: docs/verification/exe-slimming-20261025.json.
- NOT RERUN: unchanged core/merge/advanced file algorithms suites; earlier explicit evidence reused.
- NOT RUN: visual/DPI, clean-PC, real private sources/authentication or unrelated
  hardware. No GitHub publication by user instruction.


## Latest verification: text / folder / three-way workflows (2026-10-25)

Earlier sections below are dated historical evidence. Current behavior/limits
are defined in SPEC and the latest HANDOFF record.

- PASS: canonical Release and Debug application builds; Debug/Release UI test builds and Release core test build, no reported compiler warnings/errors.
- PASS: `NativeCoreTests.exe --merge-sync`, 47 focused assertions: exact mixed-EOL display/save conversion, Unicode ignore-case, two-way and three-way reconstruction (300 + 2,000 randomized cases), independent/conflicting edits, empty/deletion/boundary cases, bounded large-region fallback, linear grouping of large interleaved edits, pre-allocation line limits and cancellation.
- PASS: same core filter covers recursive byte-content folder scans, update/mirror previews, selected-directory expansion, recoverable backups, stale-plan rejection, cancellation, dirty-output guard, traversal/overlap rejection, Unicode globs, atomic result save and bounded/strict workspace INI parsing.
- PASS: `NativeUiTests.exe --advanced`, 52 assertions in each Debug/Release: actual multiline editor Save As with mixed EOL preservation; selected merge/Undo/Redo; manual-output protection; canceled render rollback; late-cancel commit adoption; stale input invalidation; source/target path + hard-link identity locks; owner-isolated unlock; three-way auto/conflict/manual resolution; zoom; exact folder paths; preview Cancel/Apply; nested update/mirror backup; pair opening and workspace save/restore without losing drafts.
- PASS: shared GUI bundles, 118 assertions per Debug/Release, selected because common layout/text/save paths changed. Existing native editing, search/replace, codecs, binary comparisons, million-row navigation, splitters, zoom/font lifetime and async-save/deferred close paths remained valid.
- PASS: `--workspace`, 35 assertions per Debug/Release; file retention/drop, independent snapshots, path Browse dialogs, minimum-size bounds, command routing and close/save decisions remained valid.
- PASS: `--workspace-process <formal-exe>` against each exact final Release/Debug binary, 11 checks each. Byte-identical normal/renamed isolated copies, concurrent one-instance startup, all file arguments retained, individual close and normal exit/relaunch.
- PASS: exact final Release/Debug binaries independently passed isolated no-argument startup/frame/normal-close smoke, exit 0.
- PASS: inspected native test-owned window renders for two-way, three-way conflicts and folder comparison. Corrected/recaptured LF-only output rendering and clipped column/mode labels. Captures are under `artifacts/evidence/advanced-native-20261025`.
- PASS: production dependency inspection shows only Windows system libraries; static MFC/CRT contract retained. Delivery root contains EXE plus the three original INIs, whose hashes match the pre-layout manifest.
- Reused: unchanged cleanup-script preservation audit and unaffected low-level document/search/codec core evidence. This is not a new clean audit or a new full core run.
- Final range/newline refinements reran focused core and Debug/Release advanced UI checks plus exact final EXE process/startup checks. Earlier shared GUI/workspace checks from this pass remain applicable to unchanged behavior; they were not repeatedly rerun after these core refinements.
- NOT RUN: physical Explorer/file-picker/wheel gestures, manual DPI/cross-monitor inspection, old-version peer emulation or power-loss/SMB injection. Automated control/message/render checks are distinct from those checks.

Pre-slimming Release (2026-10-25): `build/SimpleEditorTool.exe`, 3,151,872 bytes, SHA-256
`AC5665FCAE1B588AB72BE3447E4903839558AA829B4BEDA0E79B16A5EA60F887`.
Full file/change explanations, verification commands and limits are in HANDOFF,
README and SPEC; machine-readable evidence is `artifacts/evidence/advanced-verification.json`.

## Earlier verification history

- PASS: Release x64 MFC build, 0 warnings and 0 errors.
- PASS: Debug x64 MFC build, 0 warnings and 0 errors.
- PASS: native core test executable.
- PASS: insertion-aware, equal-address, and cooperative-cancellation core paths.
- PASS: command-line file opening and clean shutdown smoke test.
- PASS: startup window placement reported as `SW_SHOWMAXIMIZED` (`showCmd=3`).
- PASS: no Python source or bytecode files remain.

Final executable: `build\SimpleEditorTool.exe`.

## Recommendation completion verification (2026-10-24)

- PASS: one-click Release x64 build and separate Debug x64 build, no compiler warnings/errors reported.
- PASS: native core regression tests, including searches across a 1 MiB chunk boundary, invalid/overflowing 64-bit addresses, snapshot independence, expanding/deleting Replace All with one Undo/Redo, failed Open preservation, and Save As path adoption.
- PASS: UTF-8/UTF-16 BOM and newline round trips, CP950 encoding, rejection of lossy conversion, embedded NUL and truncated UTF-16, and odd-caret UTF-16 search alignment.
- PASS: native MFC UI tests in Release and Debug, using test-owned temporary files and controls; no production mutex or window identity.
- PASS: actual edit-control changes commit from Text to Hex, and Hex changes refresh Text; saving clears both modification states.
- PASS: Find dialog dispatch, wraparound Find Next, Replace, Replace All, Undo/Redo, Go To dialog, previous/next difference wrapping, virtual row address formatting, both splitter drag handlers, resize/tab retention, and keyboard/scroll synchronization.
- PASS: 1,000,000 virtual comparison rows, explicit truncation label, direct navigation to the last row, and keyboard isolation within the search dialog.
- PASS: cancellation applies no partial replacement; document revisions discard stale search results; edits clear old comparison addresses; CP950 text searches do not match inside Chinese characters.
- PASS: Release and Debug EXEs open README from the command line, report maximized placement (`showCmd=3`), and close normally with exit code 0.
- PASS: Release PE dependencies contain only Windows system components; no dynamic MFC, VC runtime, Python or Qt dependency.
- PASS: clean-preservation audit with nested delivery and junction rejection; 17 fixture files and 22 actual build files preserved by SHA-256. 124 reproducible compiler files removed.
- Reused: earlier single-instance forwarding test evidence; instance-lock handling is unchanged.
- NOT RUN: manual 125% DPI / cross-monitor visual inspection. Automated native controls/messages are tested; physical pointer behavior has not been manually inspected.

Historical recommendation-pass size: 2,848,256 bytes. SHA-256:
`BF7C40FB682BE2B679A7D4C99E8B65ADC7E7E989687466D98988B7FEB9167BE2`.

## File safety pass (2026-10-24)

- PASS: full native core bundle after changing shared document reads, snapshots, save commits and both comparison modes.
- PASS: focused `NativeCoreTests.exe --file-safety`, including same-size external edits, path replacement/deletion, failed-save byte/history retention, safe Save As recovery, destination sharing-lock failure, retry and temporary/backup cleanup.
- PASS: both comparison modes terminate on mid-run truncation; empty-file completion is 100%; aligned callbacks stay at most 4096 rows; cancellation stops before a second batch; opened-handle comparison uses the original displayed version after a path replacement.
- PASS: Debug and Release MFC tests, including actual unsaved Reload Cancel/discard confirmation, text refresh, source-change status reporting and stale comparison invalidation. Existing edit/search/splitter/million-row cases also passed in the bundled runner.
- PASS: one-click Release and separate production Debug builds, with no warnings/errors in build output.
- PASS: both application EXEs open README maximized (`showCmd=3`) and shut down gracefully with exit code 0.
- PASS: PE import inspection still shows only Windows system components; no dynamic MFC/VC runtime/Python/Qt dependency.
- Reused: earlier single-instance forwarding and clean-preservation checks. Their implementation was unchanged, so those checks were not rerun.
- NOT RUN: power-loss/disk-full injection, native partial-replacement fault codes, SMB/network storage, manual 125%/cross-monitor visual checks.

The initial open-destination save regression failed with Win32 5 and was fixed
using native ReplaceFileW plus backup. All subsequent core and UI checks passed.
Source-change detection uses identity/length/write-time metadata; concurrent
same-metadata writes are not guaranteed detectable. Saving remained synchronous
at this earlier pass; the background-save verification below supersedes that limitation.

Historical file-safety Release size: 2,822,144 bytes. SHA-256:
`91C1077802923F22EE26C351F41743730921A45CFB4D723E08761A6BFB712BFC`.

## Background-save verification (2026-10-24)

- PASS: `NativeCoreTests.exe --async-save`; cancellation at start, mid-write and pre-commit; monotonic progress; unchanged destination and retained history after cancellation; temporary/backup cleanup; post-commit cancellation reports success; destination modification in the pre-commit progress callback aborts safely; original document remains readable during a paused worker; retained-handle Save As recovery works asynchronously.
- PASS: `NativeCoreTests.exe --file-safety`; existing external-change, locked-destination, save retry, handle recovery and comparison I/O regressions are preserved.
- PASS: Debug and Release full MFC UI runners. Save is asynchronous; edits and encoding changes are gated; scroll/layout remain available; successful Save restores editing and preserves the caret; Save As adopts the path.
- PASS: failed/cancelled saves retain edits, cancel pending Open/Close and allow retry; late cancellation adopts committed disk bytes while cancelling pending close; unsaved Open/Close confirmations resume only after success; queued Open, Reload and comparison-mode continuations work.
- PASS: native Open/comparison file-picker dialogs are shown after save success and safely dismissed by the fixture. Save-and-close commits before frame teardown. Test waits validate HWND/permanent object identity before reusing frame pointers.
- PASS: existing Text/Hex, search/replace, difference navigation, splitter, source-change and million-row tests in the bundled MFC runner.
- PASS: one-click Release and separate Debug builds; no warnings/errors reported. Both production EXEs open README maximized and exit normally with code 0.
- PASS: current PE imports contain only Windows system components. No Python source or additional runtime/dependency was introduced.
- Reused: unchanged clean-script preservation and production single-instance evidence, not rerun.
- NOT RUN: power-loss/disk-full/native partial-replacement injection, SMB latency, physical gesture automation, manual 125%/cross-monitor DPI. Encoding/snapshot preparation is still synchronous; cancellation waits for an in-progress OS I/O call to return.

Historical background-save Release size: 2,834,944 bytes. SHA-256:
`4424768E6F3CCE534D0684C6168C61744A26F38851265EEEEBF2ABA5C9864D95`.

## Delivery layout verification (2026-10-24)

- PASS: all 33 original build files preserved by SHA-256 immediately after relocation, before normal rebuilding/cleaning of reproducible compiler outputs.
- PASS: `build` contains exactly one Release EXE and the three existing INIs; no folders, PDBs or screenshots. All INI hashes match the pre-change inventory after build, smoke and clean.
- PASS: Release/Debug production builds and all three project XML files. Debug EXE uses `artifacts/debug`; core and both UI test configurations build under `artifacts/tests`; linker PDBs use `artifacts/obj`.
- PASS: Release core `--file-safety` and Debug UI `--async-save` at the relocated paths. Release UI was built, not executed in this pass.
- PASS: hash-matched isolated Release/Debug startup, owned MFC frame creation and normal exit 0 with fresh fixture settings. Final Release was rebuilt and smoke-tested after clean.
- PASS: shared clean static scan (3 scripts), non-destructive help, fixture idempotence (17 protected files), and junction rejection before deletion.
- PASS: actual clean removed 135 allowlisted compiler files while retaining 15 protected files under delivery/object roots and 11 additional files under other artifact directories by SHA-256.
- PASS: Windows-system-only Release PE imports, unchanged static MFC/static CRT deployment, and Git ignore rules covering development artifacts without ignoring the delivery EXE.
- SIZ-01/02/03: baseline EXE preserved under `artifacts/baseline/layout-before`; inventories under `artifacts/evidence`. No optimization/resource/runtime/security settings changed. Production EXE remains 2,834,944 bytes; EXE plus existing INIs remains 2,835,667 bytes. Delivery-root file bytes changed from 56,034,371 to 2,835,667 by relocating PDB/screenshots, not by shrinking linked code.
- SIZ-04: paired performance benchmarking N/A; output paths only, with no compiler/runtime/feature optimization or performance claim.
- Reused: unchanged full edit/search/comparison and production single-instance evidence. Full suites and manual visual/DPI checks were not rerun for directory relocation.

Historical delivery-layout Release: `build/SimpleEditorTool.exe`, 2,834,944 bytes, SHA-256
`796ADBFA1773608206ECA510716ECAFECE98A0BA3DEDF4D0663BFD286982B783`.

## Wheel zoom verification (2026-10-24)

- PASS: production one-click Release and separate Debug builds; final builds report no compiler warnings/errors.
- PASS: `NativeUiTests.exe --zoom` in Debug and Release, 34 checks each. Plain wheel adjusts 10% per notch, partial deltas accumulate, bounds stay 50%-300%, and per-tab values/percentages survive tab switches.
- PASS: wheel routing from actual child HWNDs and queued wheel input when tab-strip focus remains; Shift+wheel scrolls Hex/Text/list without changing scale; Ctrl+0 resets the active tab.
- PASS: Hex caret/top address retention and zoomed byte hit testing; file bytes and dirty state remain unchanged. Text font really changes while preserving draft, revision, modification state, selection, first visible line and Undo.
- PASS: both comparison panes and difference list use the shared Compare percentage; address columns grow to fit. Existing rows, selection and pane scroll synchronization survive zoom.
- PASS: independent zoom/progress status panes and idle command updates; 30 repeated font/resize/repaint/tab cycles retain valid borrowed fonts without GDI handle growth.
- PASS: Debug and Release `--async-save`, including wheel zoom during Save with unchanged snapshot revision/caret, existing read-only guards, commit/cancel/error and deferred Open/Close/Compare paths.
- PASS: final hash-matched isolated Release/Debug native startup/frame/normal-close smoke, both exit 0. Existing INIs retain their pre-layout SHA-256 hashes. Delivery root contains exactly EXE plus three INIs.
- Reused: prior single-instance, clean preservation and unrelated full core/search/codec checks. Core algorithms, clean scripts and runtime linkage were unchanged; those suites/audits were not rerun.
- NOT RUN: physical wheel gestures or manual 100%/125%/cross-monitor DPI visual inspection; computer-use `node_repl` is unavailable. Automated message/font tests do not establish physical/DPI coverage.
- Session scope: zoom is per-tab in memory; no restart persistence or INI keys were requested/added.

Historical wheel-zoom Release: `build/SimpleEditorTool.exe`, 2,845,696 bytes, SHA-256
`15AEE9A411112C9C24904C9550B7845A9B26A7316A3CE8B01FF6DF244C248489`.

## Retained workspace / comparison path verification (2026-10-24)

- PASS: final native Release/Debug production builds and Debug/Release UI test builds.
- PASS: final `--workspace` in both configurations, 35 checks each: multi-file native HDROP retains drafts/Undo/selection/zoom; duplicate-path activation; missing-file preservation; independent unsaved Text/Hex snapshot comparison; Browse open/Cancel; Swap/Recompare; refreshed path choices; minimum-size control bounds; and modeless dialog lifetime.
- PASS: active-session command/shortcut routing, per-tab close/discard/Cancel, opening another file during Save, save-failure exit cancellation, cancelled queued exit, reopening a saving/queued-close tab, inactive-tab close preserving the active file, and bounded multi-path message validation.
- PASS: final `--async-save` in Debug and Release after changing saved-close dispatch. Existing save success/error/cancel/deferred-action behavior and MFC teardown remain valid.
- PASS: full shared GUI bundles in Debug and Release earlier in this pass because common frame/menu/layout changed. Search, editing, codecs, splitters, million-row navigation and zoom remained valid. Later close-state edits reran affected workspace/save cases only.
- PASS: final `--workspace-process <formal-exe>` for both Release and Debug. Byte-identical isolated normal/renamed copies launch concurrently, keep one workspace, retain three unique file tabs, close only the selected tab, exit normally and relaunch. Both exact EXEs also pass isolated fresh no-argument startup/frame/normal-close, exit 0.
- Reused: unchanged native core algorithm and clean-preservation evidence. No compiler optimization, framework/CRT dependency or cleanup policy was changed.
- Historical limits (superseded by the latest record): comparison sessions were read-only snapshots without line-aligned diff, merge, folder sync or persistent workspace restore. SPEC tracks the current scope.
- NOT RUN: physical drag/file-picker selection gestures, manual 100%/125%/cross-monitor DPI, old-version peer emulation and power-loss/SMB injection. Native HDROP/dialog/control and formal-process tests do not imply those checks.

Historical Release: `build/SimpleEditorTool.exe`, 2,877,952 bytes, SHA-256
`ED1AD9C8591001B13DBB5130241ECA9A36D9E9948F63D705CA9D62AA8A03FB8D`.
