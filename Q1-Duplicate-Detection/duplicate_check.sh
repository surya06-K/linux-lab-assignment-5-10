#!/bin/bash
# ============================================================
# Q1: Duplicate Submission Detection & Backup Script
# Usage: ./duplicate_check.sh <submissions_dir> <backup_dir>
# ============================================================

SUBMISSIONS_DIR="$1"
BACKUP_DIR="$2"

REPORT_FILE="report.txt"
DUPLICATES_LOG="duplicates.log"
ERROR_LOG="errors.txt"
HASH_DB="hashes.txt"

if [ -z "$SUBMISSIONS_DIR" ] || [ -z "$BACKUP_DIR" ]; then
    echo "Usage: $0 <submissions_dir> <backup_dir>"
    exit 1
fi

if [ ! -d "$SUBMISSIONS_DIR" ]; then
    echo "Error: Submissions directory '$SUBMISSIONS_DIR' not found." >> "$ERROR_LOG"
    exit 1
fi

mkdir -p "$BACKUP_DIR" 2>> "$ERROR_LOG"

: > "$HASH_DB"
: > "$ERROR_LOG"
: > "$DUPLICATES_LOG"

total_files=0
duplicate_files=0
backed_up_files=0

for file in "$SUBMISSIONS_DIR"/*; do
    if [ -f "$file" ]; then
        total_files=$((total_files + 1))

        hash=$(shasum -a 256 "$file" 2>>"$ERROR_LOG" | awk '{print $1}')

        if [ -z "$hash" ]; then
            echo "Error: Could not read/hash '$file' (permission or I/O issue)" >> "$ERROR_LOG"
            continue
        fi

        if grep -q "^$hash " "$HASH_DB" 2>>"$ERROR_LOG"; then
            duplicate_files=$((duplicate_files + 1))
            echo "Duplicate: $file (same content already backed up)" >> "$DUPLICATES_LOG"
        else
            echo "$hash $file" >> "$HASH_DB"
            if cp "$file" "$BACKUP_DIR"/ 2>>"$ERROR_LOG"; then
                backed_up_files=$((backed_up_files + 1))
            fi
        fi
    fi
done

{
    echo "==== Submission Processing Report ===="
    echo "Run date        : $(date)"
    echo "Source folder   : $SUBMISSIONS_DIR"
    echo "Backup folder   : $BACKUP_DIR"
    echo "Total files processed : $total_files"
    echo "Duplicate files found : $duplicate_files"
    echo "Unique files backed up: $backed_up_files"
} > "$REPORT_FILE"

cat "$REPORT_FILE"
