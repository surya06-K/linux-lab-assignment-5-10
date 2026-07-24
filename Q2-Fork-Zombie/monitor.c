/* ============================================================
   Q2: Web server child process monitor
   - Creates child "worker" processes with fork()
   - Monitors their execution with a timeout
   - Prevents zombies via a SIGCHLD handler that reaps immediately
   - Terminates unresponsive children: SIGTERM first, SIGKILL if ignored
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#define NUM_CHILDREN 4
#define TERM_TIMEOUT 3
#define KILL_TIMEOUT 5

typedef struct {
    pid_t pid;
    time_t start_time;
    int finished;
    int term_sent;
} child_info_t;

child_info_t children[NUM_CHILDREN];
volatile sig_atomic_t reaped_count = 0;

void sigchld_handler(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < NUM_CHILDREN; i++) {
            if (children[i].pid == pid) {
                children[i].finished = 1;
            }
        }
        reaped_count++;
    }
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    printf("[Parent %d] Launching %d worker children\n", getpid(), NUM_CHILDREN);

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            if (i == NUM_CHILDREN - 1) {
                printf("[Child %d] PID %d: simulating an UNRESPONSIVE request\n", i, getpid());
                signal(SIGTERM, SIG_IGN);
                while (1) sleep(1);
            } else {
                int work_time = (i + 1);
                printf("[Child %d] PID %d: handling request, expect %ds\n", i, getpid(), work_time);
                sleep(work_time);
                printf("[Child %d] PID %d: request completed normally\n", i, getpid());
                exit(0);
            }
        } else {
            children[i].pid = pid;
            children[i].start_time = time(NULL);
            children[i].finished = 0;
            children[i].term_sent = 0;
        }
    }

    int active = NUM_CHILDREN;
    while (active > 0) {
        sleep(1);
        active = 0;
        for (int i = 0; i < NUM_CHILDREN; i++) {
            if (children[i].finished) continue;
            active++;

            time_t elapsed = time(NULL) - children[i].start_time;

            if (elapsed >= KILL_TIMEOUT && children[i].term_sent) {
                printf("[Parent] PID %d unresponsive to SIGTERM -> sending SIGKILL\n", children[i].pid);
                kill(children[i].pid, SIGKILL);
            } else if (elapsed >= TERM_TIMEOUT && !children[i].term_sent) {
                printf("[Parent] PID %d exceeded %ds -> sending SIGTERM\n", children[i].pid, TERM_TIMEOUT);
                kill(children[i].pid, SIGTERM);
                children[i].term_sent = 1;
            }
        }
    }

    printf("[Parent] All children accounted for. Reaped via SIGCHLD: %d/%d\n",
           reaped_count, NUM_CHILDREN);
    return 0;
}
