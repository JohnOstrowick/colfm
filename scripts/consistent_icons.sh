#!/usr/bin/env bash
# consistent_icons.sh — normalise PNG icons to 128x128 px @ 96 dpi on black.
# Usage:
#   ./scripts/consistent_icons.sh icons
#   ./scripts/consistent_icons.sh            # defaults to "."

set -euo pipefail

inroot="${1:-.}"
outroot="$inroot/consistent_sizes"

# Choose ImageMagick
if command -v magick >/dev/null 2>&1; then
  CNV=(magick)
  ID=(magick identify)
elif command -v convert >/dev/null 2>&1 && command -v identify >/dev/null 2>&1; then
  CNV=(convert)
  ID=(identify)
else
  echo "Error: ImageMagick not found." >&2
  exit 1
fi

mkdir -p "$outroot"

echo "Input : $inroot"
echo "Output: $outroot"

count=0
# Process all PNGs under inroot, excluding the output dir and .git
while IFS= read -r -d '' f; do
  rel="${f#"$inroot"/}"
  outdir="$outroot/$(dirname "$rel")"
  mkdir -p "$outdir"
  out="$outdir/$(basename "$rel")"

  tmp="$(mktemp --suffix=".png")"
  printf '%s\n' "$((++count)): $rel"

  # 1) Flatten alpha onto black, trim fuzzy black border, set 96 dpi, height <= 128 (no upscaling)
  "${CNV[@]}" "$f" \
    -background black -alpha remove -alpha off \
    -bordercolor black -border 1 -fuzz 2% -trim +repage \
    -units PixelsPerInch -density 96 \
    -resize "x128>" \
    "$tmp"

  # 2) Crop/pad centred to exactly 128x128
  w=$("${ID[@]}" -format "%w" "$tmp") || w=0
  h=$("${ID[@]}" -format "%h" "$tmp") || h=0
  if (( w > 128 || h > 128 )); then
    "${CNV[@]}" "$tmp" -gravity center -crop 128x128+0+0 +repage "$out"
  else
    "${CNV[@]}" "$tmp" -background black -gravity center -extent 128x128 "$out"
  fi

  rm -f "$tmp"
done < <(find "$inroot" -type f -iname '*.png' \
           ! -path "$outroot/*" ! -path '*/.git/*' -print0)

if (( count == 0 )); then
  echo "No PNG files found under: $inroot"
else
  echo "Done. Processed $count file(s). Output in: $outroot"
fi
