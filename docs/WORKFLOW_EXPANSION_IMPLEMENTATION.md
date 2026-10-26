# Approved workflow expansion

User authorization: implement the ten recommendations approved on 2026-10-26.
This is an engineering ledger; product instructions remain in README and guides.

## Scope and acceptance

| ID | Approved first-version behavior | State |
| --- | --- | --- |
| W01 | Full file-source character highlights and display-only aligned gaps, including leading gaps; original text, copying, search and line numbers retained; explicit precision bounds | Verified |
| W02 | Persistent, deduplicated Text external-change notice with keep/compare/reload decisions; preserve drafts, existing reload confirmation and Hex source guards | Verified |
| W03 | Explicit Text/Hex selection mapping for current supported encodings, BOM and original EOL; reject partial/unmappable bytes and stale results | Verified |
| W04 | Optional read-only full Base pane for three-way comparison, synchronized original-line navigation and independent output | Verified |
| W05 | Named path-free comparison-setting presets; explicit save/apply, bounded portable INI, no automatic compare/sync or draft discard | Verified |
| W06 | Optional live and explicit Unicode notices for a small defined invisible/bidi set, allowlist and codepoint details; preserve characters/Undo and ordinary CJK | Verified |
| W07 | Native distribution/entropy plots from existing analysis, physical-range navigation with version checks | Verified |
| W08 | Bounded Base64/Hex/UTF-8 conversion preview, full formatted-result copy and explicit guarded decoded-byte application as one Undo | Verified |
| W09 | Native JSON ignored pointers and explicitly keyed arrays, typed unique identifiers and ambiguity errors; raw input preserved, no JS runtime | Verified |
| W10 | Read-only unsigned/signed 64-bit LEB128 inspector, length/overflow/EOF handling, endian-independent interpretation | Verified |

## Implementation plan

1. Reuse the existing source/model/search adapters for W01/W04, then connect
   external-change decisions and explicit selection mapping (W02/W03).
2. Add bounded preset/Unicode models and native commands (W05/W06).
3. Add native plot/conversion dialogs and JSON/LEB128 core extensions (W07-W10).
4. Run focused core and Debug/Release GUI checks for changed flows and affected
   callers. Preserve canonical product optimization, security, static MFC/CRT,
   pinned Scintilla/Lexilla, independent drafts, generation guards and native Undo.
5. Build the portable Release, verify the formal EXE and INIs, retain concise
   evidence, run verified compiler cleanup and remove this run's owned scratch.

No third-party source reuse, new installed runtime/driver, publication or Git
history changes. Existing edits belong to prior work and must be retained.
Keep source models immutable when changing display alignment. Source decoration
must not become clipboard/search content or modify reported original line numbers.
Do not increase existing file/line quotas merely to add these operations.

## Baseline and retention

Whole project: 13,810,043 bytes; 1,185 files, artifacts absent.
Product: 3,573,248 bytes, SHA-256
0F8D74D3511EBA7DE36092F90FA39F1C1E65885EFDCE91E0C16A25B07AFD1BCF.
Existing three delivery INIs are retained byte-for-byte unless explicitly changed
by the user. Temporary build/test growth is permitted by current shared policy;
clean completed batches and finish at or below 20,000,000 bytes.
Scratch: artifacts/workflow-expansion-20261026, fresh owner marker and project-local
TEMP/TMP. Never delete project-external or unowned data, or follow reparse points.

## Verification

Implementation and final Release verification complete.
Debug workflow expansion passes all connected flows, including source search,
character indicators, Base visibility, native preset save/apply/delete, mapping
staleness, Unicode detail navigation, external keep/compare/cancel/reload decisions,
statistics range/version guards, conversion Undo/read-only and JSON rules.

An actual Debug GDI assertion in Scintilla's squiggle indicator for a zero-width
glyph was reproduced with a project-owned debugger. Unicode notices now use a
plain native underline plus representations; vendor code and assertions are
unchanged. The failing case passes with the real modal review and paint path.
Disposable diagnostics are retired during this run's cleanup.

Four relevant core modes and nine UI modes pass in Debug/Release. The final
dialog refinement reran expansion mode in both configurations; other relevant
source behavior is unchanged. Formal EXE process and seven CLI cases pass.
Three Release images were inspected, including resized conversion/statistics
windows and visible zero-entropy baseline/selection feedback.

Final checks, limits, binary/source/configuration hashes, the newer retained main
runtime INI, protected-file preservation and complete-folder bytes are recorded in
docs/verification/workflow-expansion-20261026.json. Physical clipboard and mixed-DPI
interoperability, maximum-quota throughput and injected failures remain NOT RUN.
