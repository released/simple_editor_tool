# Document editing and comparison

[繁體中文使用指南](USER_GUIDE.md) · [English user guide](USER_GUIDE_EN.md)

The **Review** menu contains the document tools below. Original comparison
sources remain read only. Saving a merge, review metadata or report is explicit.

| Command / control | Use |
| --- | --- |
| Left / Right / Base encoding | Decode each file independently. The summary shows the actual codec, BOM and original newline types after Compare. Swap sides also swaps Left/Right decoding. Output initially uses the Left codec. |
| Align selected source lines | In a completed two-way file comparison, select an original line in each source pane, then add the pair. Press Compare to use it. Pink gutter arrows identify anchors. |
| Manage alignment points | Remove selected pairs, or remove all pairs and Apply. Cancel leaves the original list intact. Press Compare after changes. |
| Refresh selected folder rows | Re-evaluate 1–128 selected file/folder rows, recursively within selected folders when recursive scanning is enabled. Other rows are marked **not rescanned**. |
| Compare selection with clipboard | Compare the current Text selection against Unicode clipboard text in a new pasted-text tab. Existing tabs and edits are retained. |
| Choose syntax mode | Override automatic highlighting for a normal file's Text view and shared clone. Automatic returns to extension mappings; Plain text disables the lexer. |
| Syntax extension associations | Enter one `extension=language` per line, for example `.conf=JSON` and `.inc=C/C++`. Preferences remember the mappings. A document override takes priority. |
| Function sidebar / refresh | Supported code files automatically show a function list at the left of Text. The command refreshes that panel without a modal dialog. |
| Previous / Next differing file | From a folder child comparison, follow the parent's current visible order. Existing paired tabs retain their output drafts. |
| Return to parent folder comparison | Activate the folder tab that opened this pair. |
| Annotate selected difference | Add a note and optional Reviewed state to a changed two-way/three-way text block. Review state does not resolve merge conflicts. |
| Save / Load review notes | Explicitly store or load `.setreview` metadata, containing source fingerprints, block IDs and notes rather than source buffers. |
| Export annotated HTML | Export full Base/Left/Right source text with inline notes. This report contains document contents. Choose a separate output file. |
| Clear review notes | Explicitly discard the current session's annotations after confirmation. |

## Bounds and state safety

Each selection/clipboard input is limited to 16 MiB of UTF-8 and 200,000 lines.
Embedded NUL or invalid Unicode is rejected without trimming or adding a tab.
Clipboard text is read only when the command is invoked. Pasted comparisons and
their content are excluded from saved workspace files.

Alignment accepts at most 128 strictly increasing line pairs. Display gaps are
not source lines. An anchored comparison is a read-only inspection view; remove
all points and compare again to return to regular merge editing. It supports
two-way files, not pasted inputs or three-way alignment.

Selected folder refresh uses the same roots, recursion, exclusions and comparison
rules as the previous scan. Changed options require a full Compare. Refreshed
rows can be explicitly copied after preview and source revalidation. Copying
older rows, Sync All, policy synchronization and baseline capture require a full
Compare first. Parent navigation stops at the visible list ends; it does not
wrap. After a parent rescan/refresh, select a pair again in that parent. Closing
the parent leaves child documents open and disables parent navigation.

Syntax overrides are stored in explicitly saved workspaces; extension mappings
are portable preferences for normal file editors. Supported modes are C/C++,
Python, JSON, HTML/XML, PowerShell, Markdown, SQL, YAML, Shell, Batch, Diff/Patch,
CMake and Plain text. These reuse the bundled lexers and do not load plugins.

Supported code files automatically show **Functions** at the left of Text.
Click an entry or select it and press Enter to jump to its original line. Drag
the vertical divider to resize the panel. Every document owns its list and width;
switching workspace tabs shows that document's functions and preserves drafts,
caret and Undo. Shared Text views use one list and navigate in the active view.
Hex/Compare pages and ordinary data/documents retain their own layout. Use
**Review > Function sidebar / refresh** to refresh the panel.

| Source type | Recognized subset |
| --- | --- |
| C/C++ and headers | Definitions and declarations, ordinary constructors/destructors, inline and pure/default members. MFC message-map macros are excluded. Header extensions include `.h`, `.hh`, `.hpp`, `.hxx`, `.h++`, `.inl`, `.ipp` and `.tpp`. |
| Python | Named `def` / `async def`, including methods and nested definitions. |
| JavaScript / TypeScript | Named functions, common assigned arrow functions, methods and interface/class signatures; `.js`, `.jsx`, `.mjs`, `.cjs`, `.ts`, `.tsx`. |
| Java / C# | Ordinary methods, constructors and typed declarations. |
| PowerShell / Shell | Function/filter blocks; common shell `name() {}` / `function name` forms. |
| CMake | `function` and `macro` declarations. |
| SQL | Unquoted routine names in `CREATE [OR REPLACE / OR ALTER] FUNCTION/PROCEDURE/PROC`. |
| Batch | Subroutine labels, excluding `::` comments and `:eof`. |

Syntax overrides/associations determine the parser. Plain text disables the
panel. JSON, Markdown, XML/HTML, YAML and diff formats have no function sidebar;
embedded HTML scripts are not analyzed. Edits invalidate old navigation
immediately, then refresh after a brief idle delay using the unsaved editor
snapshot. Completed jobs are accepted only for the same path, revision,
generation and language. Closing a file cancels/joins its own analysis.

These are bounded heuristics, not full compilers/ASTs or preprocessing engines.
Complex decorators/attributes, operators/templates, macro-generated functions,
conditional code, dynamic definitions, computed property names, template-string
expressions and complex here documents may be omitted or interpreted imperfectly.
At most 32 MiB of native editor text, 4,096 entries and one million tokens are
considered; C/C++ nesting and matched parameter nesting are capped at 1,024.
Partial results are explicitly labelled. Analysis never rewrites or saves text.

The large-text pager has its own **Find...** and **Find next** buttons, `Ctrl+F`
and `F3`. It streams exact, case-sensitive literal searches in UTF-8 or UTF-16
LE/BE. Queries contain 1–4,096 UTF-16 code units; matches may cross read chunks.
Ctrl+F starts from the current page, F3 continues to EOF without wrapping, and
Esc cancels while retaining the current page. This pager does not provide regex,
replacement or legacy-code-page searching. Ordinary wheel scrolling and
Ctrl+wheel zoom remain independent.

Review notes allow 1,024 changed blocks and 4,096 UTF-16 code units per note.
They are bound to the exact decoded source text, original line endings and block
mapping. Changing inputs/options can leave them from an older comparison; save
or explicitly clear them before annotating a new comparison. Mismatched metadata
is refused and existing notes remain. Merge choices do not change note identity.
Notes are session data until explicitly saved; normal close asks about unsaved
notes. Workspace files do not save notes, and crash recovery does not recover
unsaved annotations. Metadata/report writes protect source paths and hard links.
