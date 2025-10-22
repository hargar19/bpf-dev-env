// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Experiment 2: Array Map - Single Lookup (Read-Only)
 * 
 * Tests the overhead of a single BPF_MAP_TYPE_ARRAY lookup operation.
 * This is the fastest map type and serves as baseline for map access overhead.
 */
#include <linux/bpf.h>
#include <linux/types.h>
#include <bpf/bpf_helpers.h>

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4096);
    __type(key, __u32);
    __type(value, __u64);
} test_map SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int trigger_syscall_prog(void *ctx) {
    __u32 key = 0;
    __u64 *value;
    
    // Single lookup operation
    value = bpf_map_lookup_elem(&test_map, &key);
    if (value) {
        // Read the value but don't modify
        // This prevents the compiler from optimizing away the lookup
        __u64 v = *value;
        (void)v; // Suppress unused variable warning
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
