# Text reports and unified patches

In a text or three-way comparison, choose **Compare > Export text report /
unified patch...**. HTML shows complete source line pairs, line numbers and EOL
labels. The text summary counts changed blocks, equal lines, left removals and
right additions; replacing a line contributes to both removal and addition
counts. The report compares the original Left/Right snapshots independently of
merged-output choices. Neither input can be the report destination.

HTML and summaries honor active two-way comparison rules and identify ignored
blocks separately. Their source cells retain raw text. Unified patches always
describe unfiltered raw changes. Manually anchored read-only comparisons require
clearing anchors and comparing again before report export, so a report cannot
silently use different alignment. See [comparison rules](EDITOR_AND_COMPARE_WORKFLOWS.md).

Unified patches contain three lines of context and preserve the presence or
absence of a final newline. Patch content compares LF-normalized text. Use the
separate output encoding/EOL conversion command to change CRLF/LF/CR styles.
Generated patches include a leading `# SimpleEditorTool normalized-sha256:`
comment; this tool checks that hash before applying a generated patch. The hash
covers normalized text, including final-newline presence, rather than file paths
or a signature. Other patch tools can ignore the comment.

Choose **Edit > Apply unified patch to text...** with an editable Text or merged
output buffer. Validation runs against the current buffer and checks hunk
positions, context, deletion text, line counts and final-newline markers. For a
generated patch it also checks the normalized source hash. A changed buffer
discards an older plan. Review the validated preview, then apply it as one Undo
action. Saving is a separate operation.

Unmodified/replaced lines retain their existing line styles. Inserted lines use
the source's first newline style (LF for an empty source). File-header paths are
metadata; this workflow updates only the selected buffer. It accepts one-file
unified patches, with exact positions/context and the English
`\ No newline at end of file` marker. Multi-file, fuzzy/context-relocation and
binary patch formats are rejected. A creation patch requires an empty buffer.

Inputs use the same 200,000-line text comparison limit. Patch files opened by the
application are limited to 32 MiB. HTML streams full lines independently of the
source-list preview limit. Cancellation preserves an existing report before
replacement and preserves the editor before patch application.

Format reference: [GNU Diffutils unified format and incomplete lines](https://www.gnu.org/s/diffutils/manual/diffutils.html).
