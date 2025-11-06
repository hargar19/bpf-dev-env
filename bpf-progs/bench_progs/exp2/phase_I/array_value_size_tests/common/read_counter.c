/*
 * Simple utility to read the op_counter map
 * Usage: ./read_counter
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <stdint.h>

int main(int argc, char **argv)
{
    int map_fd;
    uint32_t key = 0;
    uint64_t percpu_counts[128]; // Support up to 128 CPUs
    uint64_t total_ops = 0;
    
    // Find the map by name
    map_fd = bpf_obj_get("/sys/fs/bpf/op_counter");
    if (map_fd < 0) {
        // Try to find it in the loaded maps
        fprintf(stderr, "Map not pinned at /sys/fs/bpf/op_counter\n");
        fprintf(stderr, "This tool requires the BPF program to be loaded.\n");
        return 1;
    }
    
    // Read per-CPU values
    if (bpf_map_lookup_elem(map_fd, &key, percpu_counts) != 0) {
        fprintf(stderr, "Failed to read counter: %s\n", strerror(errno));
        return 1;
    }
    
    // Sum across all CPUs
    int num_cpus = libbpf_num_possible_cpus();
    printf("Per-CPU counts:\n");
    for (int cpu = 0; cpu < num_cpus; cpu++) {
        if (percpu_counts[cpu] > 0) {
            printf("  CPU %d: %lu\n", cpu, percpu_counts[cpu]);
        }
        total_ops += percpu_counts[cpu];
    }
    
    printf("\nTotal operations: %lu\n", total_ops);
    
    return 0;
}
