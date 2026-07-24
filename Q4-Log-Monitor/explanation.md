# Q4 – Real-Time Log Monitor: Explanation

## What was done
A shell script (`log_monitor.sh`) continuously watches a log file, displays
every new line the instant it's appended, simultaneously extracts only the
ERROR lines into a separate report file, and suppresses irrelevant noise
from the underlying commands.

## Step-by-step explanation

**./log_monitor.sh server.log (Tab A)** — Started the monitor. It printed a
header and then sat waiting, since `tail -f` blocks and waits for new data
rather than exiting after reading the current content.

**echo ... >> server.log (Tab B)** — Appended 6 lines (mix of INFO/ERROR)
one at a time from a second terminal tab, simulating a live application
writing to its log file while the monitor watches it.

**(Tab A) live output** — Each line appeared in Tab A the instant it was
appended in Tab B, proving real-time streaming rather than a one-time
snapshot read.

**Ctrl+C (Tab A)** — Sent SIGINT to the foreground pipeline, stopping the
monitor. Because this runs in a real interactive terminal with job control,
SIGINT reaches every process in that pipeline (tail, tee, and grep),
cleanly shutting all of them down together.

**cat error_report.txt** — Confirmed the separate report file contains
exactly the 2 ERROR lines and nothing else, proving the extraction and
separation worked correctly throughout the live run, not just at the end.

## Justification of pipes, grep, tail, redirection, and /dev/null

- **tail -f** follows a file and blocks, printing new lines to stdout the
  instant they're appended, rather than reading the file once and exiting
  (which is what plain `cat` or `tail` without `-f` would do). This is the
  mechanism that makes "display new entries in real time" possible at all.
- **Pipes (|)** connect `tail -f`'s continuous output stream directly into
  `tee`, and `tee`'s second copy into `grep`, without ever writing an
  intermediate temp file — each line flows through all three commands as
  soon as it's produced, which is both simpler and far more efficient than
  polling the file repeatedly from a script.
- **tee** duplicates a single stream into two destinations simultaneously:
  its normal stdout (this terminal, for requirement 1) and, via process
  substitution `>(...)`, a second independent copy fed into `grep` (for
  requirements 2 and 3). Without `tee`, the stream could only go to one
  place — either the screen or the filter, not both at once.
- **grep --line-buffered "ERROR"** filters the duplicated stream down to
  only lines containing "ERROR". `--line-buffered` matters specifically
  here because `grep`'s default output buffering is block-based when its
  output isn't a terminal (i.e. when piped/redirected, as it is here into
  a file) — without it, matched lines could sit in an internal buffer for
  a while before actually being written, defeating the "real-time"
  requirement for the report file. `--line-buffered` forces each match to
  be written out immediately.
- **Redirection (>>)** appends grep's matches directly into `error_report.txt`
  ("maintain a separate report file"). Because grep's stdout is redirected
  straight into this file, its matched lines are never also printed to the
  screen — there is no duplicate output to manage, since the redirection
  itself already routes that data away from the terminal.
- **/dev/null (2>/dev/null on tail)** discards `tail`'s own diagnostic
  stderr messages (for example, if the log file were briefly missing or
  got rotated out from under it) so only genuine log content reaches the
  screen — this is what "suppresses unnecessary output" in this design:
  keeping the live view clean of tool-level chatter that isn't part of the
  actual log data being monitored.

## A design mistake worth noting
An earlier version of this script also tried to add `> /dev/null` after
grep's `>> error_report.txt` redirection, intending to stop grep's matches
from printing to the screen a second time. This was based on a wrong
assumption: once grep's stdout is redirected to a file, there is no
separate screen output left to suppress. Worse, chaining two redirections
on the same file descriptor (`>> file > /dev/null`) means only the *last*
one takes effect — so that extra `/dev/null` silently discarded all of
grep's matches instead of writing them to the file, leaving the report
empty despite the live view working correctly. Removing that redundant
redirection fixed it. This is a useful illustration of why order and
scope of shell redirections matter: each redirection for the same
descriptor overrides the previous one, it does not add to it.
