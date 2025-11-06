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

    // Find and attach the requested program by name (works with any skeleton)
    struct bpf_program *prog = bpf_object__find_program_by_name(skel->obj, prog_name);
    if (!prog) {
        fprintf(stderr, "Program '%s' not found in BPF object\n", prog_name);
        fprintf(stderr, "Available programs in this skeleton:\n");
        struct bpf_program *p;
        bpf_object__for_each_program(p, skel->obj) {
            fprintf(stderr, "  - %s\n", bpf_program__name(p));
        }
        err = -ENOENT;
        goto cleanup;
    }

    struct bpf_link *link = bpf_program__attach(prog);
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
