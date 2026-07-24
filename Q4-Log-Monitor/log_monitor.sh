#!/bin/bash
# ============================================================
# Q4: Real-time log monitor
# Usage: ./log_monitor.sh <logfile>
# ============================================================

LOGFILE="$1"
REPORT="error_report.txt"

if [ -z "$LOGFILE" ]; then
    echo "Usage: $0 <logfile>"
    exit 1
fi

touch "$LOGFILE"
: > "$REPORT"

echo "Monitoring '$LOGFILE' in real time. ERROR lines are also appended to '$REPORT'."
echo "Press Ctrl+C to stop."
echo "----------------------------------------------------------"

# tail -f streams new lines as they're appended (real-time display).
# 2>/dev/null suppresses tail's own diagnostic messages so they don't
# clutter the live view -- "suppress unnecessary output".
#
# tee splits that single stream into two destinations at once:
#   1) its default stdout -> straight to this terminal (live view of
#      EVERY new line)
#   2) a second copy fed via process substitution into grep, which
#      filters only ERROR lines and appends them into the report file
# Because grep's own stdout is redirected into the report file, its
# matches are written there only -- never printed to the screen twice.
tail -f "$LOGFILE" 2>/dev/null | tee >(grep --line-buffered "ERROR" >> "$REPORT")
