#!/usr/bin/env bash
# make_icons_xml.sh — turn a folder of PNG icons into XML with base64-embedded data.
# Usage:
#   ./scripts/make_icons_xml.sh                 # read from ./, write ./icons.xml
#   ./scripts/make_icons_xml.sh icons           # read from ./icons, write ./icons.xml
#   ./scripts/make_icons_xml.sh icons out.xml   # read from ./icons, write ./out.xml
#
# Notes:
# - Recurses through the input folder.
# - Embeds PNGs as <icon name="…" filename="…" type="image/png">base64…</icon>
# - Base64 is single-line (no newlines).

set -euo pipefail

inroot="${1:-.}"
outpath="${2:-icons.xml}"

# Gather PNGs (recursive), ignore hidden output paths if re-run
mapfile -d '' files < <(find "$inroot" -type f -iname '*.png' -print0)

# Start XML
{
  printf '%s\n' '<?xml version="1.0" encoding="UTF-8"?>'
  printf '%s\n' '<icons>'
} >"$outpath"

count=0
for f in "${files[@]}"; do
  ((count++))
  # Make path relative to CWD for readability
  rel="$(realpath --relative-to="." "$f" 2>/dev/null || echo "$f")"

  # Icon name: basename without extension, lowercased, spaces->underscores
  name="$(basename "$f")"
  name="${name%.*}"
  name="$(echo "$name" | tr '[:upper:] ' '[:lower:]_')"

  # Encode as single line base64 (portable: remove all newlines)
  b64="$(base64 < "$f" | tr -d '\n')"

  {
    printf '  <icon name="%s" filename="%s" type="image/png">' "$name" "$rel"
    printf '%s' "$b64"
    printf '%s\n' '</icon>'
  } >>"$outpath"
done

# Close XML
printf '%s\n' '</icons>' >>"$outpath"

echo "Wrote $count icon(s) to $outpath"
