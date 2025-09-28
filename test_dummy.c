#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <time.h>

#ifndef __NR_dummy
#define __NR_dummy 470
#endif

#define NUM_CALLS 1000000
#define WARMUP_CALLS 1000

static inline long long timespec_to_ns(struct timespec *ts) {
    return ts->tv_sec * 1000000000LL + ts->tv_nsec;
}

int main(void) {
    struct timespec start, end;
    long long total_ns = 0;
    long ret;
    int i;

    printf("=== Dummy Syscall Baseline Performance Test ===\n");
    printf("Syscall number: %d\n", __NR_dummy);
    printf("Test calls: %d\n", NUM_CALLS);
    printf("Warmup calls: %d\n\n", WARMUP_CALLS);

    // First verify syscall works
    ret = syscall(__NR_dummy);
    if (ret == -1) {
        perror("dummy syscall failed");
        return 1;
    }
    if (ret != 0) {
        printf("ERROR: Expected return value 0, got %ld\n", ret);
        return 1;
    }
    printf("✓ Syscall verification passed (returned %ld)\n\n", ret);

    // Warmup phase
    printf("Running warmup (%d calls)...\n", WARMUP_CALLS);
    for (i = 0; i < WARMUP_CALLS; i++) {
        syscall(__NR_dummy);
    }
    printf("✓ Warmup complete\n\n");

    // Performance measurement
    printf("Starting performance measurement...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (i = 0; i < NUM_CALLS; i++) {
        ret = syscall(__NR_dummy);
        // Quick check - if syscall starts failing, abort
        if (ret != 0 && i % 100000 == 0) {
            printf("Warning: syscall returned %ld at iteration %d\n", ret, i);
        }
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);

    total_ns = timespec_to_ns(&end) - timespec_to_ns(&start);
    
    double avg_ns = (double)total_ns / NUM_CALLS;
    double avg_ms = avg_ns / 1000000.0;
    double avg_sec = avg_ns / 1000000000.0;
    
    printf("✓ Performance test complete\n\n");
    printf("=== BASELINE RESULTS ===\n");
    printf("Total calls:           %d\n", NUM_CALLS);
    printf("Total time:           %lld ns (%.3f ms)\n", total_ns, total_ns / 1000000.0);
    printf("Average per call:\n");
    printf("  %.2f ns\n", avg_ns);
    printf("  %.6f ms\n", avg_ms);
    printf("  %.9f sec\n", avg_sec);
    printf("Calls per second:     %.0f\n", (double)NUM_CALLS * 1000000000.0 / total_ns);
    printf("Calls per microsecond: %.2f\n", (double)NUM_CALLS / (total_ns / 1000.0));
    printf("========================\n");

    return 0;
}