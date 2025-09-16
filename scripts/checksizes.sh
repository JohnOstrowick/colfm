# From ./ — recursively list PNGs with size & DPI, flagging mismatches from 128×128 @ 96 dpi
mapfile -d '' files < <(find . -type f -iname '*.png' -print0); i=0; \
for f in "${files[@]}"; do
  ((i++))
  read -r w h xdpi ydpi unit < <(identify -units PixelsPerInch -format '%w %h %x %y %U' "$f")
  status="OK"; [[ "$w" -ne 128 || "$h" -ne 128 || "${xdpi%.*}" -ne 96 || "${ydpi%.*}" -ne 96 ]] && status="MISMATCH"
  printf '%3d: %s | %sx%s px | %s×%s %s | %s\n' "$i" "${f#./}" "$w" "$h" "$xdpi" "$ydpi" "$unit" "$status"
done
