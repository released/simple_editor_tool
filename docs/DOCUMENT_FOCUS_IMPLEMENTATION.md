# Document editing and comparison implementation

Approved 2026-10-26: F01-F09 from the latest competitive review. The user's
document focus excludes F10 raw pixels as well as dynamic plugins, remote access,
process memory, interpreters and raw devices. No Git publication is requested.

Entry implementation: one shared comparison encoding, textual manual anchors,
whole-folder rescans, independent paste comparison, extension-based lexers,
Markdown-only outline, bounded text pages, independent folder child sessions and
no text-difference review notes. No blocking user decision remains.

Plan before source edits:
1. F01: independent Left/Right/Base codecs and decoded summaries; retain legacy
   shared-encoding restore/defaults, independent merge-output encoding, source
   protection and explicit recompare. Cover text and structured callers.
2. F02: add selected-original-source-line anchor and a native list to remove/clear
   points. Keep 128 monotonic pairs, readonly anchor comparison and explicit
   Compare. Do not count virtual display gaps or reset an output silently.
3. F03: bounded selected file/subtree refresh using existing folder evaluation;
   validate roots/reparse/relative paths, merge atomically, retain rows outside the
   selected scope and clearly identify their older status. Keep sync revalidation.
4. F04: selected Text versus Unicode clipboard command creates a new memory-only
   comparison, preserving tabs/Undo and existing 16-MiB/200,000-line input bounds.
   Tests inject text without changing the user's real clipboard.
5. F05: manual supported syntax mode and extension associations, per-document
   override/workspace persistence, shared-view consistency and safe Save As.
   Reuse the 12 pinned lexers; no new lexer/plugin/runtime downloads.
6. F06: cancellable bounded C/C++ definition outline, snapshot/version guarded
   navigation and native dialog reuse. Explicit heuristic subset; no full AST or
   preprocessor/template expansion. Preserve existing Markdown outline.
7. F07: large-page Find/Find Next, streaming literal UTF-8/UTF-16 matches and page
   jumps; bounded query, correct cross-chunk/alignment, cancellation and source
   validation. No whole-file load, regex or large-file Replace in this version.
8. F08: folder-parent/child IDs, previous/next differing file and return-to-parent
   commands. Activate existing paired tabs or append, never replace a draft;
   reject closed/changed parents. Do not persist stale pointers.
9. F09: bounded text-block notes/review state, separate from merge choices;
   snapshot fingerprints, explicit metadata Save/Load and annotated HTML export.
   Stale notes never attach to different inputs. No automatic notes publication,
   source contents in notes, or bypass of unresolved-conflict/source-write guards.
10. Integrate native menus/catalogs and both full user guides; keep README purely
    product-facing. Preserve unrelated modifications and original runtime INIs.

Verification: meaningful new core/UI modes plus affected codec/workspace/search/
preferences/merge/folder/report callers; Debug and Release MFC lifecycle; canonical
Release and hash-matched isolated formal process/CLI checks. Reuse unchanged
algorithm/cleanup-fixture evidence. Do not edit shared build inputs during builds.

Retention: fresh artifacts/document-focus-20261026 owner, project-local TEMP/TMP
restored in finally, baseline EXE/INI copies until preservation is checked. Track
producers and expected hashes; retire canonical test EXEs and all owned scratch,
run verified safe clean/purge, preserve delivery and necessary concise evidence,
and finish below 20,000,000 bytes including hidden/Git/vendor files. Cleanup never
leaves this project; unknown/changed files are retained and reported.

Status: F01-F09 source integration and affected verification pass in Debug and
Release, including bilingual controls, independent codecs, retained drafts/Undo,
stale notes, selected refresh and >32-MiB streaming search. Six core and eleven
native UI modes per configuration, canonical Release, twelve formal workspace
checks and seven CLI exit cases pass. A reproduced report/baseline state mix-up
is fixed and its three affected UI modes pass in both configurations. Formal EXE
is 3,882,496 bytes; full records are in verification/document-focus-20261026.json.
Final clean/purge and expected-hash scratch retirement pass: 837 compiler files,
ten test/MSBuild outputs and 149 owned scratch files removed. All 1,266 protected
hashes matched both cleanup checkpoints. artifacts is absent and the complete
project is about 15.31 MB, below 20,000,000 bytes. F01-F09 is complete within the
documented subset and verification limits; no Git publication was performed.
