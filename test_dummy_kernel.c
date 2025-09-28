#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <string.h>

#ifndef __NR_dummy
#define __NR_dummy 470
#endif

void read_kernel_stats() {
    FILE *f = fopen("/proc/dummy_stats", "r");
    if (!f) {
        printf("Warning: Cannot read /proc/dummy_stats (kernel stats not available)\n");
        return;
    }
    
    printf("\n");
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }
    fclose(f);
}

void reset_kernel_stats() {
    FILE *f = fopen("/proc/dummy_stats", "w");
    if (f) {
        fprintf(f, "reset\n");
        fclose(f);
        printf("✓ Kernel statistics reset\n");
    }
}

int main(void) {
    int num_calls = 100000;
    int i;

    printf("=== Option B: Kernel-Side Timing Test ===\n");
    printf("Test calls: %d\n", num_calls);
    printf("Syscall number: %d\n\n", __NR_dummy);

    // Reset kernel statistics
    reset_kernel_stats();

    // Verify syscall works
    long ret = syscall(__NR_dummy);
    if (ret == -1) {
        perror("dummy syscall failed");
        return 1;
    }
    printf("✓ Syscall verification passed (returned %ld)\n", ret);

    // Read initial stats
    printf("\nInitial kernel stats:");
    read_kernel_stats();

    // Run test calls
    printf("\nRunning %d syscalls...\n", num_calls);
    
    for (i = 0; i < num_calls; i++) {
        syscall(__NR_dummy);
        
        // Show progress every 25k calls
        if ((i + 1) % 25000 == 0) {
            printf("  Completed %d calls...\n", i + 1);
        }
    }

    printf("✓ Test completed\n");

    // Read final stats
    printf("\nFinal kernel stats:");
    read_kernel_stats();

    printf("\nComparison notes:\n");
    printf("- Kernel timing measures only syscall body execution\n");
    printf("- Excludes user-to-kernel transition overhead\n");
    printf("- More precise than userspace timing\n");
    printf("- Min/Max show timing variance\n");

    return 0;
}