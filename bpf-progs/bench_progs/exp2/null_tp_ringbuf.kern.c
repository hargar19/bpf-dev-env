// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Experiment 2: Ring Buffer - Reserve and Commit
 * 
 * Tests BPF_MAP_TYPE_RINGBUF which is the modern replacement for
 * PERF_EVENT_ARRAY. Ring buffers are lock-free for single producer
 * and provide better performance for event passing.
 * 
 * This tests the reserve + commit pattern overhead.
 */
#include <linux/bpf.h>
#include <linux/types.h>
#include <bpf/bpf_helpers.h>

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024); // 256 KB ring buffer
} test_ringbuf SEC(".maps");

struct event {
    __u64 value;
};

SEC("tracepoint/syscalls/sys_enter_bpfprof")
int trigger_syscall_prog(void *ctx) {
    struct event *e;
    
    // Reserve space in ring buffer
    e = bpf_ringbuf_reserve(&test_ringbuf, sizeof(*e), 0);
    if (!e) {
        return 0;
    }
    
    // Write data
    e->value = 1;
    
    // Commit to ring buffer
    bpf_ringbuf_submit(e, 0);
    
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
