/*
 * Userspace benchmark runner (measurement-only)
 * Calls sys_bpfprof in a loop, each syscall triggers 100 map lookups in kernel.
 * Auto-detects bare metal vs VM and uses appropriate timing method.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

// Define the syscall number for bpfprof
// You may need to adjust this based on your kernel config
#ifndef __NR_bpfprof
#define __NR_bpfprof 451  // Adjust if different
#endif

static inline long sys_bpfprof(void)
{
    return syscall(__NR_bpfprof);
}

// Detect if running in a VM/QEMU by checking for hypervisor presence
static int is_virtualized(void)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) return 0;
    
    char line[256];
    int is_vm = 0;
    while (fgets(line, sizeof(line), fp)) {
        // Check for hypervisor flag or common VM identifiers
        if (strstr(line, "hypervisor") || 
            strstr(line, "QEMU") || 
            strstr(line, "KVM") ||
            strstr(line, "VMware")) {
            is_vm = 1;
            break;
        }
    }
    fclose(fp);
    
    // Also check for common VM device paths
    if (!is_vm) {
        if (access("/sys/class/dmi/id/product_name", R_OK) == 0) {
            fp = fopen("/sys/class/dmi/id/product_name", "r");
            if (fp) {
                if (fgets(line, sizeof(line), fp)) {
                    if (strstr(line, "QEMU") || 
                        strstr(line, "KVM") || 
                        strstr(line, "VirtualBox") ||
                        strstr(line, "VMware")) {
                        is_vm = 1;
                    }
                }
                fclose(fp);
            }
        }
    }
    
    return is_vm;
}

// RDTSC cycle counter for high-precision timing (bare metal)
static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__ (
        "rdtsc"
        : "=a" (lo), "=d" (hi)
    );
    return ((uint64_t)hi << 32) | lo;
}

// Serialize instruction execution before rdtsc
static inline uint64_t rdtsc_start(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__ (
        "cpuid\n\t"
        "rdtsc"
        : "=a" (lo), "=d" (hi)
        :: "%rbx", "%rcx"
    );
    return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t rdtsc_end(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__ (
        "rdtscp"
        : "=a" (lo), "=d" (hi)
        :: "%rcx"
    );
    __asm__ __volatile__ ("cpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)hi << 32) | lo;
}

// Get CPU frequency from /proc/cpuinfo (MHz)
static double get_cpu_freq_mhz(void)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) return 0.0;
    
    char line[256];
    double freq = 0.0;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "cpu MHz : %lf", &freq) == 1) {
            break;
        }
    }
    fclose(fp);
    return freq;
}

static inline double timespec_to_ns(struct timespec *ts)
{
    return (double)ts->tv_sec * 1e9 + (double)ts->tv_nsec;
}

int main(int argc, char **argv)
{
    int iterations = 1024;  // Paper uses 1024 accesses
    int repeats = 100;      // Increased to 100 for better statistics (paper-style)
    int warmup_runs = 5;    // Multiple warmup runs to stabilize caches
    char *variant_name = "unknown";
    char results_dir[] = "results";
    char filename[256];
    FILE *fp = NULL;
    time_t now;
    struct tm *tm_info;
    char timestamp[64];
    int use_rdtsc = 1;  // Default to RDTSC
    
    // Detect virtualization
    int is_vm = is_virtualized();
    if (is_vm) {
        printf("Detected virtualized environment (QEMU/KVM/VM)\n");
        printf("Using clock_gettime for reliable timing in VMs\n");
        use_rdtsc = 0;
    } else {
        printf("Detected bare metal environment\n");
        printf("Using RDTSC cycle counters for high-precision timing\n");
    }
    
    // Get CPU frequency for cycle-to-ns conversion (only needed for RDTSC)
    double cpu_freq_mhz = 0.0;
    double cycles_per_ns = 1.0;
    if (use_rdtsc) {
        cpu_freq_mhz = get_cpu_freq_mhz();
        cycles_per_ns = cpu_freq_mhz / 1000.0;
        if (cpu_freq_mhz == 0.0) {
            fprintf(stderr, "Warning: Could not detect CPU frequency, falling back to clock_gettime\n");
            use_rdtsc = 0;
        } else {
            printf("CPU frequency: %.2f MHz (%.3f cycles/ns)\n", cpu_freq_mhz, cycles_per_ns);
        }
    }
    
    if (argc > 1) {
        iterations = atoi(argv[1]);
    }
    if (argc > 2) {
        repeats = atoi(argv[2]);
    }
    if (argc > 3) {
        variant_name = argv[3];
    }

    // Create results directory if it doesn't exist
    mkdir(results_dir, 0777);
    chmod(results_dir, 0777);  // Ensure host VM can access
    
    // Generate timestamped filename
    time(&now);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
    snprintf(filename, sizeof(filename), "%s/%s_%s.csv", 
             results_dir, variant_name, timestamp);
    
    // Open result file
    fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
        // Fall back to stdout
        fp = stdout;
    } else {
        // Set permissive permissions so host VM can read
        chmod(filename, 0666);
    }

    fprintf(fp, "# Benchmark: %s\n", variant_name);
    fprintf(fp, "# Iterations: %d, Repeats: %d (warmup: %d)\n", iterations, repeats, warmup_runs);
    fprintf(fp, "# Operations per iteration: 100\n");
    fprintf(fp, "# Total operations per repeat: %d\n", iterations * 100);
    fprintf(fp, "# Environment: %s\n", is_vm ? "Virtualized (QEMU/KVM/VM)" : "Bare metal");
    fprintf(fp, "# Timing method: %s\n", use_rdtsc ? "RDTSC cycle counter" : "clock_gettime (CLOCK_MONOTONIC)");
    if (use_rdtsc) {
        fprintf(fp, "# CPU frequency: %.2f MHz\n", cpu_freq_mhz);
    }
    fprintf(fp, "# Timestamp: %s\n", timestamp);
    if (use_rdtsc) {
        fprintf(fp, "variant,iteration,total_cycles,cycles_per_syscall,cycles_per_operation,ns_per_operation\n");
    } else {
        fprintf(fp, "variant,iteration,total_ns,ns_per_syscall,ns_per_operation\n");
    }
    
    printf("Running benchmark: %d iterations x %d repeats (+ %d warmup)\n", 
           iterations, repeats, warmup_runs);
    printf("Saving results to: %s\n", filename);

    // Warmup runs (not recorded)
    for (int w = 0; w < warmup_runs; w++) {
        printf("Warmup run %d...\n", w);
        for (int i = 0; i < iterations; i++) {
            long ret = sys_bpfprof();
            if (ret < 0) {
                fprintf(stderr, "syscall failed: %ld (errno=%d)\n", ret, errno);
                if (fp != stdout) fclose(fp);
                return 1;
            }
        }
    }

    printf("Starting measurement runs...\n");
    
    for (int r = 0; r < repeats; r++) {
        if (use_rdtsc) {
            // Bare metal: Use RDTSC cycle counters
            uint64_t start_cycles, end_cycles;
            
            start_cycles = rdtsc_start();
            
            for (int i = 0; i < iterations; i++) {
                long ret = sys_bpfprof();
                if (ret < 0) {
                    fprintf(stderr, "syscall failed: %ld (errno=%d)\n", ret, errno);
                    if (fp != stdout) fclose(fp);
                    return 1;
                }
            }
            
            end_cycles = rdtsc_end();
            
            uint64_t total_cycles = end_cycles - start_cycles;
            double cycles_per_syscall = (double)total_cycles / iterations;
            double cycles_per_operation = cycles_per_syscall / 100.0;
            double ns_per_operation = cycles_per_operation / cycles_per_ns;
            
            fprintf(fp, "%s,%d,%llu,%.2f,%.4f,%.4f\n", variant_name, r, 
                    (unsigned long long)total_cycles, cycles_per_syscall, 
                    cycles_per_operation, ns_per_operation);
            printf("%s,%d,%llu,%.2f,%.4f,%.4f\n", variant_name, r, 
                   (unsigned long long)total_cycles, cycles_per_syscall, 
                   cycles_per_operation, ns_per_operation);
        } else {
            // VM: Use clock_gettime for reliable wall-clock timing
            struct timespec start, end;
            
            clock_gettime(CLOCK_MONOTONIC, &start);
            
            for (int i = 0; i < iterations; i++) {
                long ret = sys_bpfprof();
                if (ret < 0) {
                    fprintf(stderr, "syscall failed: %ld (errno=%d)\n", ret, errno);
                    if (fp != stdout) fclose(fp);
                    return 1;
                }
            }
            
            clock_gettime(CLOCK_MONOTONIC, &end);
            
            double start_ns = timespec_to_ns(&start);
            double end_ns = timespec_to_ns(&end);
            double total_ns = end_ns - start_ns;
            double ns_per_syscall = total_ns / iterations;
            double ns_per_operation = ns_per_syscall / 100.0;
            
            fprintf(fp, "%s,%d,%.2f,%.2f,%.4f\n", variant_name, r, 
                    total_ns, ns_per_syscall, ns_per_operation);
            printf("%s,%d,%.2f,%.2f,%.4f\n", variant_name, r, 
                   total_ns, ns_per_syscall, ns_per_operation);
        }
    }

    if (fp != stdout) {
        fclose(fp);
        printf("\nResults saved to: %s\n", filename);
    }

    return 0;
}
