# Q5 – vi/vim Crash Recovery Mechanisms: Evaluation

## Scenario
A developer is editing a critical configuration file in vi/vim. The system
crashes before the file is saved. This was reproduced exactly: a real vim
session was killed with `kill -9` (a hard, unclean termination with zero
chance for vim to run its normal quit/cleanup code) while an edit was sitting
unsaved in the buffer.

## What was demonstrated

**Swap file recovery (hands-on, verified):**
1. Edited `nginx.conf` in vim, added a line, did not save.
2. Killed the vim process with `kill -9` from a second terminal -- a genuine
   crash simulation, not `:q`.
3. Confirmed the file on disk was unchanged (the edit was gone), but a hidden
   `.nginx.conf.swp` swap file had survived the crash.
4. Reopening the file with `vim nginx.conf` triggered vim's own crash
   detection: an `E325: ATTENTION` warning stating "An edit session for this
   file crashed" and offering `(R)ecover`.
5. Pressing `R` restored the unsaved line from the swap file, confirmed by
   vim's own "Recovery completed" message and by visually inspecting the
   buffer. Saving after recovery made the fix permanent.

**Backup file (`~`) mechanism (hands-on, verified, for comparison):**
1. Enabled `:set backup`, made and saved an edit (`new_line_1`).
2. Made a second edit (`new_line_2`) and saved again.
3. `sample.conf~` was created containing the version *before* the second
   save (with `new_line_1` but not `new_line_2`) -- proving backup files
   protect the previous saved version from being lost by a bad save, but
   only trigger at save time.

## Evaluation of all five recovery mechanisms

**1. Swap files (`.filename.swp`)**
Written to disk continuously *while editing* (vim flushes it periodically
and after pauses in typing), independent of whether the user ever saves.
This is the only mechanism in this list that is actually designed to
survive an abrupt crash mid-edit, because it doesn't depend on any graceful
shutdown step happening first. Demonstrated directly above.

**2. Auto-recovery (`vim -r`, or the automatic ATTENTION prompt)**
This is not a separate storage mechanism -- it's the *retrieval* mechanism
for the swap file. Vim automatically detects a leftover swap file the next
time the same file is opened and offers to reconstruct the buffer from it.
Its reliability is entirely inherited from the swap file's reliability,
since that's what it reads from.

**3. Backup files (`filename~`)**
Only created at the moment of a `:w` (save), and only if `:set backup` (or
`backupcopy`/`writebackup`) is enabled -- it is NOT vim's default behavior.
A backup captures the *previous saved version* right before it gets
overwritten. This protects against saving something in error and wanting
to roll back, but it does nothing for content that was never saved at
all -- exactly the situation in this question (crash before any save). If
the developer had never saved even once, no backup file would exist yet.

**4. Undo history (`u`, persistent undo via `:set undofile`)**
By default, undo history lives only in memory for the current vim session.
It is lost entirely when the process is killed, since nothing is written to
disk for it unless `undofile` is explicitly enabled -- and even then, the
undo file is typically written out at normal `:w`/quit time, not
continuously during editing. A hard crash gives it no opportunity to persist,
so it is not a dependable recovery path for this scenario.

**5. Registers (yank/delete buffers, `:registers`)**
Registers exist purely in vim's in-memory state during a session. They can
be made to persist across normal, clean quits via `viminfo`
(`:wviminfo`/`shada` in newer vim), but a `kill -9` crash never reaches that
write step. Registers are the least crash-durable of all five mechanisms --
useful for moving text around within a single live session, but irrelevant
to recovering from an actual crash.

## Most reliable recovery strategy

**The swap file, accessed via auto-recovery, is the most reliable strategy
for recovering unsaved work after a crash**, and the reason comes down to
*when* each mechanism writes data to disk:

- Swap files: written continuously, throughout editing, independent of
  save/quit -- survives even mid-keystroke.
- Backup files: written only at save time -- protects saved history, not
  unsaved work.
- Undo history / registers: live in memory by default, and even their
  optional disk-persistence (`undofile`, `viminfo`) is normally flushed at
  clean quit, which a hard crash never reaches.

This was directly confirmed in the demonstration: after `kill -9`, only the
swap file was available to reconstruct the lost edit -- the on-disk config
file itself had no trace of it, and there had been no save yet for a backup
file to even exist. As a practical safeguard, a developer editing a
critical config file should keep vim's default swap-file behavior enabled
(never run with `-n` / `set noswapfile`) and know to check for and use
`vim -r filename` (or respond `R` to the ATTENTION prompt) immediately after
any crash, before making further edits to the same file, since a fresh vim
session on the same file will keep the recovery option available until the
stale swap file is deleted.
