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

// Benchmark maps: 4096 entries, varying value sizes
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4096);
    __type(key, u32);
    __type(value, struct value_8b);
} bench_array_8b SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4096);
    __type(key, u32);
    __type(value, struct value_64b);
} bench_array_64b SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4096);
    __type(key, u32);
    __type(value, struct value_256b);
} bench_array_256b SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4096);
    __type(key, u32);
    __type(value, struct value_1kb);
} bench_array_1kb SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4096);
    __type(key, u32);
    __type(value, struct value_4kb);
} bench_array_4kb SEC(".maps");

// Legacy 8-byte map for backward compatibility
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4096);
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

// Per-CPU value buffers to avoid BPF stack limit (512 bytes)
// Used for update operations - each CPU gets its own buffer
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, struct value_8b);
} value_buf_8b SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, struct value_64b);
} value_buf_64b SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, struct value_256b);
} value_buf_256b SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, struct value_1kb);
} value_buf_1kb SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, u32);
    __type(value, struct value_4kb);
} value_buf_4kb SEC(".maps");

// Baseline noop: 100 empty iterations (amortization baseline for update operations)
SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_noop(struct trace_event_raw_sys_enter *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (count) {
        #pragma unroll
        for (int i = 0; i < 100; i++) {
            __sync_fetch_and_add(count, 1);
            asm volatile("");
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

// Measurement variants: 100 UPDATES per invocation, hot key (0) to keep cache residency
// Uses bpf_map_update_elem instead of bpf_map_lookup_elem
// Loop fully unrolled for minimal control overhead.

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_update_8b(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (!count) return 0;
    
    u32 key = 0;  // Hot key
    u32 buf_key = 0;
    
    // Get value buffer from per-CPU map (avoids stack limit)
    struct value_8b *value = bpf_map_lookup_elem(&value_buf_8b, &buf_key);
    if (!value) return 0;
    
    // Initialize value once
    value->data = 0xdeadbeef;
    
#pragma unroll
    for (int i = 0; i < 100; i++) {
        __sync_fetch_and_add(count, 1);
        // UPDATE operation
        bpf_map_update_elem(&bench_array_8b, &key, value, BPF_ANY);
    }
    
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_update_64b(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (!count) return 0;
    
    u32 key = 0;  // Hot key
    u32 buf_key = 0;
    
    // Get value buffer from per-CPU map
    struct value_64b *value = bpf_map_lookup_elem(&value_buf_64b, &buf_key);
    if (!value) return 0;
    
    // Initialize value once
    #pragma unroll
    for (int j = 0; j < 8; j++) {
        value->data[j] = 0xdeadbeef;
    }
    
#pragma unroll
    for (int i = 0; i < 100; i++) {
        __sync_fetch_and_add(count, 1);
        // UPDATE operation
        bpf_map_update_elem(&bench_array_64b, &key, value, BPF_ANY);
    }
    
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_update_256b(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (!count) return 0;
    
    u32 key = 0;  // Hot key
    u32 buf_key = 0;
    
    // Get value buffer from per-CPU map
    struct value_256b *value = bpf_map_lookup_elem(&value_buf_256b, &buf_key);
    if (!value) return 0;
    
    // Initialize value once
    #pragma unroll
    for (int j = 0; j < 32; j++) {
        value->data[j] = 0xdeadbeef;
    }
    
#pragma unroll
    for (int i = 0; i < 100; i++) {
        __sync_fetch_and_add(count, 1);
        // UPDATE operation
        bpf_map_update_elem(&bench_array_256b, &key, value, BPF_ANY);
    }
    
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_update_1kb(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (!count) return 0;
    
    u32 key = 0;  // Hot key
    u32 buf_key = 0;
    
    // Get value buffer from per-CPU map
    struct value_1kb *value = bpf_map_lookup_elem(&value_buf_1kb, &buf_key);
    if (!value) return 0;
    
    // Initialize value once
    #pragma unroll
    for (int j = 0; j < 128; j++) {
        value->data[j] = 0xdeadbeef;
    }
    
#pragma unroll
    for (int i = 0; i < 100; i++) {
        __sync_fetch_and_add(count, 1);
        // UPDATE operation
        bpf_map_update_elem(&bench_array_1kb, &key, value, BPF_ANY);
    }
    
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int bench_update_4kb(void *ctx)
{
    u32 counter_key = 0;
    u64 *count = bpf_map_lookup_elem(&op_counter, &counter_key);
    if (!count) return 0;
    
    u32 key = 0;  // Hot key
    u32 buf_key = 0;
    
    // Get value buffer from per-CPU map
    struct value_4kb *value = bpf_map_lookup_elem(&value_buf_4kb, &buf_key);
    if (!value) return 0;
    
    // Initialize value once
    #pragma unroll
    for (int j = 0; j < 512; j++) {
        value->data[j] = 0xdeadbeef;
    }
    
#pragma unroll
    for (int i = 0; i < 100; i++) {
        __sync_fetch_and_add(count, 1);
        // UPDATE operation
        bpf_map_update_elem(&bench_array_4kb, &key, value, BPF_ANY);
    }
    
    return 0;
}
