# Q3 – Secure File Processing with Raw Syscalls: Explanation

## What was done
A C program (`emp_records.c`) manages a binary file of fixed-size employee
records using only low-level syscalls — `open()`, `read()`, `write()`,
`lseek()`, `close()` — instead of buffered stdio functions like `fopen`/
`fread`. Because every record is exactly the same size, any record's byte
offset can be computed directly (`index * sizeof(Employee)`), which is
what makes in-place updates and direct lookups possible without touching
the rest of the file.

## Step-by-step explanation

**gcc -Wall -o emp_records emp_records.c** — Compiled with all warnings
enabled; zero warnings.

**./emp_records** — Ran the full demo in one pass:
- Step 1 created `employees.dat` and wrote 5 records (60 bytes each,
  300 bytes total — confirmed by `ls -la`).
- Step 2 read all 5 records sequentially to show the initial state.
- Step 3 updated only record index 2 (Kiran Rao's salary, 38000 -> 55000)
  by seeking directly to byte offset 120 and overwriting just that
  60-byte record — the other 240 bytes of the file were never touched.
- Step 4 retrieved record index 4 directly by seeking to byte offset 240,
  without reading records 0-3 first, proving random access works.
- Step 5 re-read all 5 records sequentially and confirmed only record 2's
  salary changed; every other record was byte-for-byte identical to Step 2.

**ls -la employees.dat / xxd employees.dat** — Verified the file is exactly
300 bytes (5 x 60-byte records) and inspected the raw hex bytes, confirming
this is genuine fixed-width binary storage, not a text file.

## Justification of open(), read(), write(), lseek(), close()

- **open()** — `open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644)` creates
  the file fresh in Step 1 (`O_CREAT` makes it if missing, `O_TRUNC` clears
  any old content, `0644` sets rw-r--r-- permissions). Later steps reopen
  the same file with `O_RDONLY` (safe, read-only lookups) or `O_RDWR`
  (needed for Step 3 since it both seeks and writes). Using the specific
  flag combination needed for each operation follows the principle of
  least privilege — e.g. the lookup steps can never accidentally corrupt
  the file because they only hold a read-only descriptor.
- **write()** — Writes the raw bytes of an `Employee` struct directly to
  the file descriptor's current position. Because every struct is the
  same fixed size (60 bytes here, due to struct padding), every `write()`
  call places a record at a predictable offset, which is the foundation
  the whole random-access scheme depends on.
- **lseek()** — This is the key syscall for "update/retrieve without
  rewriting the entire file." `lseek(fd, index * sizeof(Employee),
  SEEK_SET)` moves the file descriptor's read/write cursor directly to
  the byte where a given record starts, in O(1) time, regardless of file
  size or how many records precede it. This is what let Step 3 overwrite
  only record 2, and Step 4 read only record 4, without processing any
  other record.
- **read()** — Pulls exactly `sizeof(Employee)` bytes from the current
  file position into a struct in memory. The sequential steps (2 and 5)
  rely on the fact that `read()` automatically advances the file position
  after each call, so calling it in a loop naturally walks through every
  record in order.
- **close()** — Releases the file descriptor after each open/operate
  cycle. The program deliberately opens and closes the file separately
  for each logical operation (create, read-all, update, lookup, read-all
  again) rather than holding one descriptor open throughout, which
  mirrors how a real multi-request service would handle each file
  operation as an independent, self-contained unit of work.
