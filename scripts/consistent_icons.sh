#!/usr/bin/env bash
# consistent_icons.sh — normalise PNG icons to 128x128 px @ 96 dpi on black.
# Usage:
#   ./scripts/consistent_icons.sh icons
#   ./scripts/consistent_icons.sh                 # defaults to "."

set -euo pipefail

inroot="${1:-.}"
outroot="consistent_sizes"

# Pick ImageMagick 7 or 6
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

# Gather PNGs recursively under inroot, excluding our output dir and .git
mapfile -d '' files < <(
  find "$inroot" -type f -iname "*.png" \
    ! -path "*/$outroot/*" ! -path "*/.git/*" -print0
)

if (( ${#files[@]} == 0 )); then
  echo "No PNG files found under: $inroot"
  exit 0
fi

i=0
for f in "${files[@]}"; do
  ((i++))
  # Make a relative path against inroot
  rel="${f#"$inroot"/}"
  # If the file is exactly the inroot (no slash), keep basename only
  [[ "$f" == "$inroot" ]] && rel="$(basename "$f")"

  outdir="$outroot/$(dirname "$rel")"
  mkdir -p "$outdir"
  out="$outdir/$(basename "$rel")"

  tmp="$(mktemp --suffix=".png")"
  echo "$i: $f  ->  $out"

  # 1) Flatten onto black, trim fuzzy black border, set 96 dpi.
  # 2) Downscale so height <= 128 (no upscaling).
  "${CNV[@]}" "$f" \
    -background black -alpha remove -alpha off \
    -bordercolor black -border 1 -fuzz 2% -trim +repage \
    -units PixelsPerInch -density 96 \
    -resize "x128>" \
    "$tmp"

  # Crop/pad to exactly 128x128 centred.
  w=$("${ID[@]}" -format "%w" "$tmp")
  h=$("${ID[@]}" -format "%h" "$tmp")

  if (( w > 128 || h > 128 )); then
    # If any side is over 128 after trim/resize, centre-crop to 128x128
    "${CNV[@]}" "$tmp" -gravity center -crop 128x128+0+0 +repage "$out"
  else
    # Otherwise pad to 128x128 on black
    "${CNV[@]}" "$tmp" -background black -gravity center -extent 128x128 "$out"
  fi

  rm -f "$tmp"
done

echo "Done. Normalised icons are mirrored under '$outroot/'."
