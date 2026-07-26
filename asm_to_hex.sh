#!/bin/bash

set -e

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <input_asm_file> <output_hex_file>"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"

if [[ ! -f "$INPUT_FILE" ]]; then
    echo "Error: Input file '$INPUT_FILE' not found"
    exit 1
fi

# TEMP_BIN="${INPUT_FILE%.asm}.bin"
TEMP_BIN="drive.img"
nasm -f bin "$INPUT_FILE" -o "$TEMP_BIN"
od -A n -t x1 -v "$TEMP_BIN" | sed 's/^ //' > "$OUTPUT_FILE"
