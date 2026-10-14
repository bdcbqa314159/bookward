# bookward

A reading catalog that lives in your terminal. First consumer of
[dataward](https://github.com/bdcbqa314159/dataward) — two structs, two
`BOOST_DESCRIBE_STRUCT` lines, SQLite underneath.

One book is one row with a generated id, and the id names the PDF on the
archive drive (`bk-0001.pdf`). The id never changes: re-reading a book adds a
reading, not a second book — including a re-read in the same year.

```sh
bookward add "Dune" --author "Frank Herbert" --edition 1st --year 2021
bookward again bk-0001 --year 2026     # read it again (same year is fine)
bookward edit bk-0001 --author "Frank Herbert"
bookward worked bk-0001 yes            # technical books: worked through? (or no / clear)
bookward find dune                     # substring search over titles and authors
bookward list --year 2026
bookward year bk-0001 2026 2025        # move a reading to another year
bookward remove bk-0001 --year 2026    # forget one reading; without --year, the whole book
bookward report 2026                   # LaTeX/PDF + Markdown catalog; no year = every year
```

Readings show as a multiset, so a book read twice in a year appears twice:
`bk-0001  Dune — Frank Herbert (1st)  [2021, 2026, 2026]`.

`report` writes a `.tex`, a `.pdf` and a Markdown twin — `reading-2026.*` for
one year, `reading-catalog.*` for all of them — to the current directory, or
to `--out <dir>`. The PDF needs `latexmk` on PATH; without it the `.tex` is
still written.

The database lives at `~/.bookward.db` (override with `BOOKWARD_DB`). All
three frontends share that one file.

## Frontends

| Binary | What it is |
| --- | --- |
| `bookward` | the CLI above |
| `bookward-tui` | FTXUI terminal UI — Books / Table / Stats tabs, edits in place |
| `bookward-gui` | egui desktop app, Rust over a cxx bridge into `bookward_core` |

The rules live in `bookward_core` exactly once; every frontend calls the same
`cmd_*` verbs, so none of them knows the schema.

## Build

```sh
cmake --preset debug
cmake --build build/debug -j
ctest --test-dir build/debug --output-on-failure
```

Presets: `debug`, `release`, `asan` (ASan+UBSan). The GUI builds through
cargo, which drives the CMake build itself:

```sh
cd rust && cargo test        # FFI smoke across the whole C++ stack
```

`scripts/dist.sh [dest]` builds release and copies the binaries to `dest`
(default `~/bin`), GUI included when cargo is available.
