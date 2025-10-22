// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Experiment 2: Hash Map - Delete Operation
 * 
 * Tests the overhead of deleting entries from a hash map.
 * Pre-populates the map and then deletes entries on each invocation.
 * 
 * Note: This requires map pre-population from userspace.
 */
#include <linux/bpf.h>
#include <linux/types.h>
#include <bpf/bpf_helpers.h>

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 4096);
    __type(key, __u64);
    __type(value, __u64);
} test_map SEC(".maps");

// Counter for cycling through keys
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u64);
} counter_map SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int trigger_syscall_prog(void *ctx) {
    __u32 zero = 0;
    __u64 *counter;
    __u64 key;
    
    // Get counter
    counter = bpf_map_lookup_elem(&counter_map, &zero);
    if (!counter) {
        return 0;
    }
    
    // Cycle through keys 0-4095
    key = (*counter) % 4096;
    (*counter)++;
    
    // Delete entry (may or may not exist)
    bpf_map_delete_elem(&test_map, &key);
    
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
