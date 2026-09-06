#!/usr/bin/env bash
# Build release and plain-copy the executables (CLI, TUI, and — when cargo is
# available — the Rust GUI) to a destination directory. No symlinks; the
# copies are fully self-contained (only OS dylibs).
#
#   scripts/dist.sh [dest]      dest defaults to ~/bin
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"
dest="${1:-$HOME/bin}"

cmake --preset release -S "$repo" > /dev/null
cmake --build "$repo/build/release" -j --target bookward bookward-tui > /dev/null

mkdir -p "$dest"
cp "$repo/build/release/bookward" "$repo/build/release/bookward-tui" "$dest/"
exes=(bookward bookward-tui)

if command -v cargo > /dev/null; then
  (cd "$repo/rust" && cargo build --release > /dev/null 2>&1)
  cp "$repo/rust/target/release/bookward-gui" "$dest/"
  exes+=(bookward-gui)
else
  echo "cargo not found — skipping the GUI"
fi

echo "installed to $dest:"
for exe in "${exes[@]}"; do
  echo "  $dest/$exe"
done
echo "dynamic deps of bookward-tui (should be OS-only):"
otool -L "$dest/bookward-tui" 2>/dev/null | tail -n +2 || true
