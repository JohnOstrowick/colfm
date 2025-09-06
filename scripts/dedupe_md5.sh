#!/bin/bash
# dupemd5.sh — report duplicate files by MD5; dry-run move + CSV log

APPLY=0
if [[ "${1:-}" == "--apply" ]]; then
    APPLY=1
    shift
fi
dir="${1:-.}"

declare -A seen
count=0

duplicates_dir="./duplicates"
csv="$duplicates_dir/duplicates_found.csv"

mkdir -p "$duplicates_dir"
touch "$csv"
if [[ ! -s "$csv" ]]; then
    echo '"item";"original";"duplicate";"target"' > "$csv"
fi

print_summary() {
    echo "Total duplicate pairs found: $count"
}

log_csv() { # idx, orig, dup, target
    local idx="$1" orig="$2" dup="$3" target="$4"
    esc() { printf '%s' "$1" | sed 's/"/""/g'; }
    printf '"%s";"%s";"%s";"%s"\n' \
        "$(esc "$idx")" "$(esc "$orig")" "$(esc "$dup")" "$(esc "$target")" >> "$csv"
}

unique_name() {
    local target="$1"
    local base="$(basename "$target")"
    local name="${base%.*}"
    local ext="${base##*.}"
    local dir="$(dirname "$target")"
    local n=1
    local candidate="$target"
    while [[ -e "$candidate" ]]; do
        n=$((n+1))
        if [[ "$ext" != "$name" ]]; then
            candidate="$dir/${name}_$n.$ext"
        else
            candidate="$dir/${name}_$n"
        fi
    done
    echo "$candidate"
}

move_dup() { # dry-run by default; real move with --apply
    local dup="$1" target="$2"
    target=$(unique_name "$target")
    if (( APPLY )); then
        mv -n -- "$dup" "$target"
    else
        echo "mv -n -- \"$dup\" \"$target\""
    fi
    echo "$target"
}

while IFS= read -r -d '' file; do
    md5=$(md5sum "$file" | awk '{print $1}')
    if [[ -n "${seen[$md5]}" ]]; then
        echo "original: ${seen[$md5]}"
        echo "apparent duplicate: $file"
        echo
        echo

        orig="${seen[$md5]}"
        dup="$file"
        target="$duplicates_dir/$(basename "$dup")" 

        orig_abs="$(realpath -m "$orig")"
        dup_abs="$(realpath -m "$dup")"
    
        target_abs="$(move_dup "$dup_abs" "$target")"
        count=$((count+1))
        log_csv "$count" "$orig_abs" "$dup_abs" "$target_abs"
    else
        seen[$md5]="$file"
    fi
done < <(find "$dir" -type f -print0 | LC_ALL=C sort -z)

print_summary
