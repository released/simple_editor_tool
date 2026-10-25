# Intel HEX and Motorola S-record tools

Use Tools to validate record checksums and inspect the decoded address map, or
convert between raw BIN and Intel HEX / Motorola S-record files. These commands
operate on files; they do not program hardware or install a driver.

Intel HEX supports data, EOF, extended segment/linear and start segment/linear
records. Segment offsets wrap within their 64-KiB segment. Linear records that
cross the end of the 32-bit address space are rejected. S-record supports
S0/S1/S2/S3/S5/S6/S7/S8/S9, including optional data-record counts and start addresses.
Malformed checksums/counts, overlapping data, unknown records, missing EOF and
data after termination are rejected with the record line number. A single input
is treated as one image; concatenated S-record modules are not accepted.

Validation accepts up to 2,000,000 records and 64 MiB of decoded data. The address
map retains sparse segments; its dialog previews 200 segments. BIN conversion
starts at the minimum data address, ends after the highest data byte, fills gaps
with the chosen byte and limits this span to 256 MiB. The address map reports
the base address; BIN itself does not retain it. BIN export to records asks for
the base explicitly. Intel output uses type 04 extended linear addresses;
Motorola output uses S3 data and an S7 termination address at the supplied base.

Start addresses in an input image appear in the validation report. A conversion
through BIN cannot preserve such metadata. Output files must be separate from
the input; cancellation and destination-state checks apply before replacement.

Format references: [Intel HEX specification](https://srecord.sourceforge.net/man/man5/srec_intel.5.html),
[Motorola S-record specification](https://srecord.sourceforge.net/man/man5/srec_motorola.5.html).
