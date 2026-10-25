# Binary field templates

Open a file in Hex mode. Choose **Tools > Open binary field template...**, select
a UTF-8 `.settpl` file and enter its physical base offset. The structure tree
groups dotted field names; select a field to see its type, physical byte range
and decoded value. **Go to field** closes the inspector and selects those bytes
in the source Hex pane. Out-of-file or invalid fields cannot navigate. Templates
also inspect either read-only binary comparison pane.

Each nonempty line has this format:

```text
path type offset [decimal-count [stride]]
```

Offsets and strides use hexadecimal by default, or `d:` for decimal. Counts are
decimal. The selected base is added to every offset. `#` begins a comment.

```text
# Fixed header at the selected base
header.magic bytes:4 0
header.version u16le 4
header.flags u16be 6
header.ratio f32le 8
header.name utf8:16 C
entries[].value u32le 20 4 8
```

The final line expands four fields at base + 0x20, 0x28, 0x30 and 0x38. Without
an explicit stride, fields are adjacent. A single `[]` in a path becomes its
array index; dotted names form groups, up to eight levels.

| Types | Interpretation |
| --- | --- |
| `u8`, `i8` | Unsigned / signed 8-bit |
| `u16/u32/u64`, `i16/i32/i64` | Integers; append `le` or `be`, default little endian |
| `f32`, `f64` | IEEE float/double; append `le` or `be` |
| `bytes:N` | Fixed byte span, first 64 bytes shown |
| `ascii:N`, `utf8:N` | Fixed byte span, text stops at its first NUL |

`N` is 1–4096. Definitions are bounded to 1 MiB UTF-8 / 512 Ki UTF-16 units,
1024 definition lines and 4096 expanded fields. Address overflow, unknown types
and invalid syntax are refused. A field beyond EOF is labeled, without reading
outside the file. Source changes cancel adoption of results.

Templates are read-only field descriptions with no expressions, loops, code
execution or automatic loading. They are not compatible with ImHex Pattern
Language. For byte distribution and block entropy, use **Tools > CRC32 / SHA-256 /
entropy...** on a selected range or the whole file.
