#!/bin/bash
# Regenerate icons_data_blob.h from icons.xml
# Usage: ./make_icons_blob.sh

set -e

INPUT="./icons/icons.xml"
OUTPUT="./helpers/icons_data_blob.h"

if [[ ! -f "$INPUT" ]]; then
  echo "Error: $INPUT not found."
  exit 1
fi

{
  echo "#pragma once"
  echo "// generated from $INPUT — do not edit"
  echo
  echo "const unsigned char kIconsXmlBlob[] = {"
  # Keep only the hex dump from xxd, strip unwanted lines
  xxd -i "$INPUT" \
    | sed '1d;$d' \
    | sed '/_len/d'
  echo "};"
  echo
  echo "const unsigned int kIconsXmlBlobLen = sizeof(kIconsXmlBlob);"
} > "$OUTPUT"

echo "Wrote $OUTPUT from $INPUT"
