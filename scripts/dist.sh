#!/usr/bin/env bash
# Build release and plain-copy the two executables to a destination directory.
# No symlinks — the copies are fully self-contained (only OS dylibs).
#
#   scripts/dist.sh [dest]      dest defaults to ~/bin
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"
dest="${1:-$HOME/bin}"

cmake --preset release -S "$repo" > /dev/null
cmake --build "$repo/build/release" -j --target bookward bookward-tui > /dev/null

mkdir -p "$dest"
cp "$repo/build/release/bookward" "$repo/build/release/bookward-tui" "$dest/"

echo "installed to $dest:"
for exe in bookward bookward-tui; do
  echo "  $dest/$exe"
done
echo "dynamic deps of bookward-tui (should be OS-only):"
otool -L "$dest/bookward-tui" 2>/dev/null | tail -n +2 || true
