# Q2 – Fork/Monitor/Zombie Prevention/Signal Termination: Explanation

## What was done
A C program (`monitor.c`) simulates a web server spawning worker child
processes to handle requests. It monitors each child's runtime, reaps
finished children immediately to avoid zombies, and forcibly terminates
any child that hangs past a timeout — escalating from SIGTERM to SIGKILL
if the child ignores the polite signal.

## Step-by-step explanation

**gcc -Wall -o monitor monitor.c** — Compiled with all warnings enabled;
zero warnings confirms no undefined behavior around signal handling or
process bookkeeping.

**./monitor** — Ran interactively. Three children (0,1,2) completed
normally after 1s/2s/3s of simulated work and exited via `exit(0)`. The
fourth child was scripted to simulate a hung request: it installs
`signal(SIGTERM, SIG_IGN)` and loops forever. The parent's monitoring
loop detected it exceeded the 3-second timeout and sent SIGTERM; since
the child ignored it, the parent detected it was still alive 2 seconds
later and escalated to SIGKILL, which cannot be caught or ignored,
terminating it immediately.

**./monitor > output.txt 2>&1** — Re-ran with output redirected to a file
to capture a permanent record. Output was identical to the interactive
run — no duplicated lines — because `setvbuf(stdout, NULL, _IOLBF, 0)`
forces line-buffering regardless of whether stdout is a terminal or a
file (see justification below).

**ps aux | grep -i defunct** — Produced no output, proving no zombie
(defunct) processes remained in the process table after all four
children were handled.

## Justification of process creation, waiting, and signal handling

- **fork()** creates each child as an exact copy of the parent at the
  point of the call; the return value (0 in the child, child's PID in
  the parent) is how each process knows which branch of the `if` to
  execute. This is why child-specific logic (simulate work vs. simulate
  a hang) lives inside the `pid == 0` branch.
- **Zombie prevention via SIGCHLD + waitpid(WNOHANG):** when a child
  exits, the kernel keeps its exit status in the process table as a
  "zombie" until the parent calls `wait()`/`waitpid()` on it. Registering
  a `SIGCHLD` handler means the kernel notifies the parent the instant
  any child exits; the handler loops on `waitpid(-1, &status, WNOHANG)`
  (non-blocking) to reap every child that has exited, which is why
  `ps aux | grep defunct` shows nothing — no child is ever left unreaped.
- **Monitoring with a timeout loop:** the parent doesn't just wait
  passively — it polls every second and compares each child's elapsed
  runtime to `TERM_TIMEOUT`/`KILL_TIMEOUT`. This is the "detect an
  unresponsive request" logic a real web server needs, since a hung
  worker won't send SIGCHLD on its own.
- **Signal escalation (SIGTERM -> SIGKILL):** `kill(pid, SIGTERM)` asks a
  process to terminate gracefully (it can be caught/ignored, as
  demonstrated by the hung child calling `signal(SIGTERM, SIG_IGN)`).
  Because real unresponsive processes may not honor SIGTERM,
  `kill(pid, SIGKILL)` is sent as a fallback — SIGKILL cannot be caught,
  blocked, or ignored by any process, guaranteeing termination.
- **stdout buffering and fork():** stdio buffers output before writing
  it. If a `printf()` call before `fork()` hasn't been flushed yet (which
  is the default when stdout is not a terminal, e.g. redirected to a
  file), the child inherits a *copy* of that unflushed buffer and will
  flush it again itself later, duplicating output. Calling
  `setvbuf(stdout, NULL, _IOLBF, 0)` forces line-by-line flushing so each
  `printf` is written out immediately and never duplicated across
  parent/child, regardless of whether output goes to a terminal or a file.
