// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Experiment 2: Hash Map - Lookup + Update
 * 
 * Tests hash map write operations with atomic increment.
 * Includes locking overhead for concurrent modifications.
 */
#include <linux/bpf.h>
#include <linux/types.h>
#include <bpf/bpf_helpers.h>

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 4096);
    __type(key, __u32);
    __type(value, __u64);
} test_map SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int trigger_syscall_prog(void *ctx) {
    __u32 key = 0;
    __u64 *value;
    
    value = bpf_map_lookup_elem(&test_map, &key);
    if (value) {
        __sync_fetch_and_add(value, 1);
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
