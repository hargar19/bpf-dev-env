/*
 * BPF loader for paper-style benchmark
 * Loads and attaches BPF programs to bpfprof tracepoint
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "array_bench.skel.h"

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    return vfprintf(stderr, format, args);
}

int main(int argc, char **argv)
{
    struct array_bench_bpf *skel;
    int err;
    const char *prog_name = "bench_lookup";  // default

    if (argc > 1) {
        prog_name = argv[1];
    }

    // Set up libbpf errors and debug output
    libbpf_set_print(libbpf_print_fn);

    // Open BPF application
    skel = array_bench_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }

    // Load & verify BPF programs
    err = array_bench_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load and verify BPF skeleton: %d\n", err);
        goto cleanup;
    }

    // Attach the requested program
    struct bpf_link *link = NULL;
    if (strcmp(prog_name, "bench_noop") == 0) {
        link = bpf_program__attach(skel->progs.bench_noop);
    } else if (strcmp(prog_name, "bench_lookup") == 0) {
        link = bpf_program__attach(skel->progs.bench_lookup);
    } else if (strcmp(prog_name, "bench_lookup_8b") == 0) {
        link = bpf_program__attach(skel->progs.bench_lookup_8b);
    } else if (strcmp(prog_name, "bench_lookup_64b") == 0) {
        link = bpf_program__attach(skel->progs.bench_lookup_64b);
    } else if (strcmp(prog_name, "bench_lookup_256b") == 0) {
        link = bpf_program__attach(skel->progs.bench_lookup_256b);
    } else if (strcmp(prog_name, "bench_lookup_1kb") == 0) {
        link = bpf_program__attach(skel->progs.bench_lookup_1kb);
    } else if (strcmp(prog_name, "bench_lookup_4kb") == 0) {
        link = bpf_program__attach(skel->progs.bench_lookup_4kb);
    } else {
        fprintf(stderr, "Unknown program: %s\n", prog_name);
        fprintf(stderr, "Available: bench_noop, bench_lookup, bench_lookup_{8b,64b,256b,1kb,4kb}\n");
        err = 1;
        goto cleanup;
    }

    if (!link) {
        err = -errno;
        fprintf(stderr, "Failed to attach BPF program '%s': %d\n", prog_name, err);
        goto cleanup;
    }

    printf("BPF program '%s' loaded and attached successfully\n", prog_name);
    
    // Pin the op_counter map for external access
    int counter_fd = bpf_map__fd(skel->maps.op_counter);
    if (counter_fd >= 0) {
        // Create /sys/fs/bpf if it doesn't exist
        system("mkdir -p /sys/fs/bpf");
        
        // Pin the map
        err = bpf_obj_pin(counter_fd, "/sys/fs/bpf/op_counter");
        if (err == 0) {
            printf("Operation counter pinned at /sys/fs/bpf/op_counter\n");
            printf("Use './read_counter' to validate operation count\n");
        } else if (errno == EEXIST) {
            printf("Operation counter already pinned\n");
        } else {
            fprintf(stderr, "Warning: Failed to pin counter map: %s\n", strerror(errno));
        }
    }
    
    printf("Press Ctrl+C to stop...\n");

    // Keep running until interrupted
    while (1) {
        sleep(1);
    }

cleanup:
    // Unpin the map
    unlink("/sys/fs/bpf/op_counter");
    array_bench_bpf__destroy(skel);
    return err != 0;
}
