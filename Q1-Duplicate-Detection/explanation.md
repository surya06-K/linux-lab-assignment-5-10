# Q1 – Duplicate Submission Detection & Backup: Explanation

## What was done
A shell script (`duplicate_check.sh`) processes a folder of student submissions,
detects files with identical content (not just identical names), backs up only
unique files, and produces a summary report while logging errors separately.

## Step-by-step explanation

**mkdir submissions / echo ... > file** — Created six sample submission files.
Two pairs share identical content (Ravi's two files, Priya's two files) to
simulate students submitting the same work twice, and one file was locked with
`chmod 000` to simulate an unreadable/corrupted submission.

**./duplicate_check.sh submissions backup** — Ran the script. It processed all
6 files, correctly identified 2 as duplicates by content hash, flagged 1 as
unreadable due to permissions, and backed up the remaining 3 unique files.

**cat report.txt / duplicates.log / errors.txt / hashes.txt** — Verified each
output file: the report shows the counts, duplicates.log lists which files
were skipped and why, errors.txt shows the real `Permission denied` error from
`shasum` on the locked file, and hashes.txt is the internal SHA-256 database
used to detect duplicates.

## Justification of commands, redirection, and file-handling techniques

- **shasum -a 256** computes a SHA-256 content hash per file. This detects
  duplicates by actual content rather than filename, so `ravi_module5.txt`
  and `ravi_copy.txt` (different names, same content) are correctly matched.
- **grep -q "^$hash "** checks whether a hash already exists in the hash
  database. `-q` suppresses output since only the exit status (found/not
  found) is needed, keeping the script efficient for hundreds of files.
- **Redirection operators**:
  - `>` is used once per fresh log (report.txt, and `: >` to truncate/reset
    hashes.txt, errors.txt, duplicates.log at the start of each run) so old
    data doesn't leak into a new run.
  - `>>` (append) is used inside the loop to add one line per file to
    hashes.txt / duplicates.log without overwriting prior entries.
  - `2>>` redirects only stderr (e.g. `shasum`'s "Permission denied") into
    errors.txt, keeping error messages fully separate from normal
    duplicate/report output — this directly satisfies the requirement to
    store error messages separately.
  - `{ ... } > report.txt` groups multiple echo commands so their combined
    stdout is redirected once, instead of reopening the file per line.
- **cp "$file" "$BACKUP_DIR"/** performs the actual backup of unique files
  only, after the duplicate check passes.
- **File-handling techniques**: `mkdir -p` ensures the backup directory
  exists without erroring if it's already present; `[ -f "$file" ]` guards
  against directories/special files being processed; `$?`/`if cp ...; then`
  checks the exit status of `cp` before incrementing the backed-up counter,
  so a failed copy is not miscounted as a successful backup.
