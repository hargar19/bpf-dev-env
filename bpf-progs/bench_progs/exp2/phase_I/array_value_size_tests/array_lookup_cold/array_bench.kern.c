/*
 * Paper-style array map benchmark - measurement-only version
 * Each attached program executes 1024 array map lookups (hot) per invocation
 * Triggered via tracepoint/syscalls/sys_enter_bpfprof
 */
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";


// Value size structures (matching paper's test matrix)
struct value_8b {
    u64 data;  // 8 bytes
};

struct value_64b {
    u64 data[8];  // 64 bytes
};

struct value_256b {
    u64 data[32];  // 256 bytes
};

struct value_1kb {
    u64 data[128];  // 1024 bytes
};

struct value_4kb {
    u64 data[512];  // 4096 bytes
};

// Benchmark maps: 16384 entries (increased from 4096 to exceed cache)
// At 16K entries: 8B=128KB, 64B=1MB, 256B=4MB, 1KB=16MB, 4KB=64MB
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 16384);
    __type(key, u32);
    __type(value, struct value_8b);
} bench_array_8b SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 16384);
    __type(key, u32);
    __type(value, struct value_64b);
} bench_array_64b SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 16384);
    __type(key, u32);
    __type(value, struct value_256b);
} bench_array_256b SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 16384);
    __type(key, u32);
    __type(value, struct value_1kb);
} bench_array_1kb SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 16384);
    __type(key, u32);
    __type(value, struct value_4kb);
} bench_array_4kb SEC(".maps");

// Legacy 8-byte map for backward compatibility
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 16384);
    __type(key, u32);
    __type(value, u64);
} bench_array SEC(".maps");

// Per-CPU counter to validate actual operations executed
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, u64);
} op_counter SEC(".maps");

// Track unique keys accessed (for cold cache validation)
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 16384);
    __type(key, u32);
    __type(value, u64);  // Access count
} key_tracker SEC(".maps");

// Baseline noop: 100 empty iterations with random key generation (cold cache baseline)
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_noop(struct trace_event_raw_sys_enter *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (count) {
        #pragma unroll
        for (int i = 0; i < 100; i++) {
            __sync_fetch_and_add(count, 1);
            // Generate random key but don't use it (baseline overhead)
            // Use iteration counter as seed mix to get different keys each time
            u32 key = (bpf_get_prandom_u32() + i * 157) % 16384;
            asm volatile("" : : "r"(key));
        }
    }

    return 0;
}// Legacy single lookup variant (not used for batched measurement, kept for completeness)
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_lookup(void *ctx)
{
    u32 key = 0; // constant hot key
    u64 *value = bpf_map_lookup_elem(&bench_array, &key);
    if (value) asm volatile("");
    return 0;
}

// Measurement variants: 100 lookups per invocation, COLD CACHE (random keys across full keyspace)
// Loop fully unrolled for minimal control overhead.
// Uses iteration-dependent keys to maximize cache thrashing
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_lookup_8b(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (count) {
#pragma unroll
        for (int i = 0; i < 100; i++) {
            __sync_fetch_and_add(count, 1);
            // Mix iteration counter with random for better distribution
            u32 key = (bpf_get_prandom_u32() + i * 157) % 16384;
            struct value_8b *value = bpf_map_lookup_elem(&bench_array_8b, &key);
            if (value) {
                asm volatile("" : : "r"(value->data));  // Force memory access (scalar)
            }
        }
    }
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_lookup_64b(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (count) {
#pragma unroll
        for (int i = 0; i < 100; i++) {
            __sync_fetch_and_add(count, 1);
            u32 key = (bpf_get_prandom_u32() + i * 157) % 16384;
            struct value_64b *value = bpf_map_lookup_elem(&bench_array_64b, &key);
            if (value) {
                asm volatile("" : : "r"(value->data[0]));  // Force memory access
            }
        }
    }
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_lookup_256b(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (count) {
#pragma unroll
        for (int i = 0; i < 100; i++) {
            __sync_fetch_and_add(count, 1);
            u32 key = (bpf_get_prandom_u32() + i * 157) % 16384;
            struct value_256b *value = bpf_map_lookup_elem(&bench_array_256b, &key);
            if (value) {
                asm volatile("" : : "r"(value->data[0]));  // Force memory access
            }
        }
    }
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_lookup_1kb(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (count) {
#pragma unroll
        for (int i = 0; i < 100; i++) {
            __sync_fetch_and_add(count, 1);
            u32 key = (bpf_get_prandom_u32() + i * 157) % 16384;
            struct value_1kb *value = bpf_map_lookup_elem(&bench_array_1kb, &key);
            if (value) {
                asm volatile("" : : "r"(value->data[0]));  // Force memory access
            }
        }
    }
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_lookup_4kb(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (count) {
#pragma unroll
        for (int i = 0; i < 100; i++) {
            __sync_fetch_and_add(count, 1);
            u32 key = (bpf_get_prandom_u32() + i * 157) % 16384;
            struct value_4kb *value = bpf_map_lookup_elem(&bench_array_4kb, &key);
            if (value) {
                asm volatile("" : : "r"(value->data[0]));  // Force memory access
            }
        }
    }
    return 0;
}
