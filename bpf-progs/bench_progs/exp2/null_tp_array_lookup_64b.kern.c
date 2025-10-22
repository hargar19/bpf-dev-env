// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Experiment 2: Array Map - Large Value (64 bytes)
 * 
 * Tests lookup overhead with larger values (64 bytes = 1 cache line).
 * This represents a common case for storing multiple fields or
 * small structs in BPF programs.
 */
#include <linux/bpf.h>
#include <linux/types.h>
#include <bpf/bpf_helpers.h>

struct large_value {
    __u64 data[8];  // 64 bytes total
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4096);
    __type(key, __u32);
    __type(value, struct large_value);
} test_map SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int trigger_syscall_prog(void *ctx) {
    __u32 key = 0;
    struct large_value *value;
    
    value = bpf_map_lookup_elem(&test_map, &key);
    if (value) {
        // Read first field to prevent optimization
        __u64 v = value->data[0];
        (void)v;
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
