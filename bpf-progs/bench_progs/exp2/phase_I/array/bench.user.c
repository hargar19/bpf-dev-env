// SPDX-License-Identifier: MIT
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/syscall.h>

#define __NR_BPFPROG 470
#define DEFAULT_TOTAL_SECS 10.0
#define DEFAULT_INTERVAL_SECS 0.5
#define DEFAULT_WARMUP_SECS 0.0

static inline double now_sec(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char* argv[]) {
    /* Args:
     *  argv[1] = total measurement seconds (excluding warm-up)
     *  argv[2] = interval seconds
     *  argv[3] = warm-up seconds (optional, default 0)
     *  argv[4] = output file (optional)
     */
    double total_secs   = (argc > 1) ? strtod(argv[1], NULL) : DEFAULT_TOTAL_SECS;
    double interval_sec = (argc > 2) ? strtod(argv[2], NULL) : DEFAULT_INTERVAL_SECS;
    double warmup_secs  = (argc > 3) ? strtod(argv[3], NULL) : DEFAULT_WARMUP_SECS;
    const char *outfile = (argc > 4) ? argv[4] : NULL;
    if (interval_sec <= 0.0) interval_sec = DEFAULT_INTERVAL_SECS;
    if (warmup_secs < 0.0) warmup_secs = 0.0;
    setvbuf(stdout, NULL, _IOLBF, 0);

    FILE *f = NULL;
    if (outfile) {
        f = fopen(outfile, "a");
        if (!f) {
            perror("fopen outfile");
        }
    }

    fprintf(stdout, "Starting array map throughput test (warm-up %.3fs, measure %.3fs, interval %.3fs)\n", warmup_secs, total_secs, interval_sec);
    fprintf(stdout, "--------------------------------------------------------------\n");
    if (f) {
        fprintf(f, "# array map throughput test warmup=%.3f total=%.3f interval=%.3f\n", warmup_secs, total_secs, interval_sec);
    }

    double t_start = now_sec();
    double t_warm_done = t_start + warmup_secs;
    double t_measure_start = 0.0; // set once warm-up ends
    double next_mark = t_start + interval_sec;
    int num_calls_interval = 0;
    long total_calls = 0;

    for (;;) {
        syscall(__NR_BPFPROG);
        double t_now = now_sec();
        if (t_now < t_warm_done) {
            // In warm-up phase: do not count, do not log intervals
            continue;
        }
        if (t_measure_start == 0.0) {
            // Transition from warm-up to measurement
            t_measure_start = t_now;
            next_mark = t_measure_start + interval_sec;
        }
        num_calls_interval++; total_calls++;
        double elapsed = t_now - t_measure_start;
        if (t_now >= next_mark) {
            fprintf(stdout, "%.3f:%d\n", elapsed, num_calls_interval);
            if (f) fprintf(f, "%.3f:%d\n", elapsed, num_calls_interval);
            num_calls_interval = 0;
            do { next_mark += interval_sec; } while (t_now >= next_mark);
        }
        if (elapsed >= total_secs) {
            double rate = (double)total_calls / elapsed;
            fprintf(stdout, "--------------------------------------------------------------\n");
            fprintf(stdout, "Total: %ld calls in %.3f s (excl. %.3fs warm-up) = %.0f calls/sec\n", total_calls, elapsed, warmup_secs, rate);
            if (f) {
                fprintf(f, "Total: %ld calls in %.3f s (excl. %.3fs warm-up) = %.0f calls/sec\n", total_calls, elapsed, warmup_secs, rate);
                fclose(f);
            }
            break;
        }
    }
    return 0;
}
