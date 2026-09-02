# bookward

A reading log that lives in your terminal. First consumer of
[dataward](https://github.com/bdcbqa314159/dataward) — one `Book` struct, one
`BOOST_DESCRIBE_STRUCT` line, SQLite underneath.

```sh
bookward add dune "Dune" --author "Frank Herbert" --pages 412
bookward progress dune 120
bookward finish dune --rating 5
bookward list --status reading
bookward report 2026        # per-year reading report (LaTeX/PDF, later)
```

The database lives at `~/.bookward.db` (override with `BOOKWARD_DB`).

## Build

```sh
cmake --preset debug
cmake --build build/debug -j
ctest --test-dir build/debug --output-on-failure
```
