// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Phase II: Array Map Throughput - Map Infrastructure Overhead
 * Measures cost of BPF map helpers under different array access patterns using
 * the test syscall tracepoint (sys_enter_bpfprof).
 * Variants implemented:
 *   array_lookup_only       : bpf_map_lookup_elem only
 *   array_lookup_update     : lookup + bpf_map_update_elem
 *   array_update_only       : direct bpf_map_update_elem (overwrite)
 *   array_multiple_lookups  : 4 consecutive lookups
 *   array_sequential_update : round-robin key selection + update
 *   array_random_update     : timestamp-derived pseudo-random key + update
 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1024);
    __type(key, __u32);
    __type(value, __u64);
} array_map SEC(".maps");

static __always_inline __u32 rr_key(void) {
    // Per-tracepoint static counter is acceptable; races just change distribution
    static __u32 counter = 0;
    return counter++ & 1023; // modulo 1024
}

static __always_inline __u32 rand_key(void) {
    __u64 ns = bpf_ktime_get_ns();
    // Mix bits a little then bound
    return ((__u32)(ns >> 10) ^ (__u32)ns) & 1023;
}

/* 1. Lookup only */
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int array_lookup_only(void *ctx) {
    __u32 key = 0;
    __u64 *val = bpf_map_lookup_elem(&array_map, &key);
    if (val) {
        // consume value to avoid dead-code removal
        __u64 tmp = *val;
        if (tmp == 0xFFFFFFFFFFFFFFFFULL) {
            // impossible branch keeps tmp 'used'
            bpf_map_lookup_elem(&array_map, &key);
        }
    }
    return 0;
}

/* 2. Lookup + Update (read-modify-write using helpers) */
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int array_lookup_update(void *ctx) {
    __u32 key = 0;
    __u64 new_val = 1;
    __u64 *val = bpf_map_lookup_elem(&array_map, &key);
    if (val)
        new_val = *val + 1;
    bpf_map_update_elem(&array_map, &key, &new_val, BPF_ANY);
    return 0;
}

/* 3. Update only (overwrite existing entry) */
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int array_update_only(void *ctx) {
    __u32 key = 0;
    __u64 val = bpf_ktime_get_ns(); // arbitrary changing data
    bpf_map_update_elem(&array_map, &key, &val, BPF_ANY);
    return 0;
}

/* 4. Multiple lookups (4 keys) */
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int array_multiple_lookups(void *ctx) {
    __u64 agg = 0;
    for (int i = 0; i < 4; i++) {
        __u32 key = i;
        __u64 *val = bpf_map_lookup_elem(&array_map, &key);
        if (val)
            agg += *val;
    }
    if (agg == 0xDEADBEEF) {
        __u32 k = 0; bpf_map_lookup_elem(&array_map, &k);
    }
    return 0;
}

/* 5. Sequential update (round-robin keys) */
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int array_sequential_update(void *ctx) {
    __u32 key = rr_key();
    __u64 new_val = key; // simple deterministic content
    bpf_map_update_elem(&array_map, &key, &new_val, BPF_ANY);
    return 0;
}

/* 6. Random update (timestamp-derived key) */
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int array_random_update(void *ctx) {
    __u32 key = rand_key();
    __u64 new_val = key ^ bpf_ktime_get_ns();
    bpf_map_update_elem(&array_map, &key, &new_val, BPF_ANY);
    return 0;
}
