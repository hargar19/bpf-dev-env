// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Experiment 2: Per-CPU Hash Map - Single Lookup
 * 
 * Tests BPF_MAP_TYPE_PERCPU_HASH which combines:
 * - Hash computation (same as regular hash)
 * - No locking (per-CPU isolation)
 */
#include <linux/bpf.h>
#include <linux/types.h>
#include <bpf/bpf_helpers.h>

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_HASH);
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
        __u64 v = *value;
        (void)v;
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
