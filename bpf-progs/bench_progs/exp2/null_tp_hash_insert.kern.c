// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Experiment 2: Hash Map - Insert Operation
 * 
 * Tests the overhead of inserting new entries into a hash map.
 * Unlike lookup, this may trigger memory allocation and hash table
 * growth, providing different performance characteristics.
 * 
 * Uses incrementing keys to avoid collisions.
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

// Per-CPU counter for unique keys
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
    __u64 value = 1;
    
    // Get per-CPU counter to generate unique keys
    counter = bpf_map_lookup_elem(&counter_map, &zero);
    if (!counter) {
        return 0;
    }
    
    // Use counter as key (unique per invocation)
    key = *counter;
    (*counter)++;
    
    // Insert new entry
    bpf_map_update_elem(&test_map, &key, &value, BPF_NOEXIST);
    
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
