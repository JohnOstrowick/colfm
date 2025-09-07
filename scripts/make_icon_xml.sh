{ 
  printf '%s\n' '<?xml version="1.0" encoding="UTF-8"?>'
  printf '%s\n' '<icons>'
  mapfile -d '' files < <(find ./icons -type f -iname '*.png' -print0)
  for f in "${files[@]}"; do
    name="$(basename "$f")"
    printf '<icon>\n<name>%s</name>\n<data>%s</data>\n</icon>\n' \
      "$name" "$(base64 -w0 "$f")"
  done
  printf '%s\n' '</icons>'
} > ../icons/icons.xml
