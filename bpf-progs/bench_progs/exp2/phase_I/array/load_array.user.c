// SPDX-License-Identifier: MIT
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h> // for sleep()
#include <bpf/libbpf.h>
#include "array_perf.skel.h"

static volatile sig_atomic_t stop;
static void on_sig(int sig) { stop = 1; }

static int libbpf_print_fn(enum libbpf_print_level level, const char *fmt, va_list args) {
    return vfprintf(stderr, fmt, args);
}

static void disable_all(struct array_perf_bpf *skel) {
    bpf_program__set_autoload(skel->progs.array_lookup_only, false);
    bpf_program__set_autoload(skel->progs.array_lookup_update, false);
    bpf_program__set_autoload(skel->progs.array_update_only, false);
    bpf_program__set_autoload(skel->progs.array_multiple_lookups, false);
    bpf_program__set_autoload(skel->progs.array_sequential_update, false);
    bpf_program__set_autoload(skel->progs.array_random_update, false);
}

int main(int argc, char **argv) {
    const char *prog = (argc > 1) ? argv[1] : "array_lookup_only";
    libbpf_set_print(libbpf_print_fn);

    struct array_perf_bpf *skel = array_perf_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open skeleton\n");
        return 1;
    }
    disable_all(skel);

    if (!strcmp(prog, "array_lookup_only"))
        bpf_program__set_autoload(skel->progs.array_lookup_only, true);
    else if (!strcmp(prog, "array_lookup_update"))
        bpf_program__set_autoload(skel->progs.array_lookup_update, true);
    else if (!strcmp(prog, "array_update_only"))
        bpf_program__set_autoload(skel->progs.array_update_only, true);
    else if (!strcmp(prog, "array_multiple_lookups"))
        bpf_program__set_autoload(skel->progs.array_multiple_lookups, true);
    else if (!strcmp(prog, "array_sequential_update"))
        bpf_program__set_autoload(skel->progs.array_sequential_update, true);
    else if (!strcmp(prog, "array_random_update"))
        bpf_program__set_autoload(skel->progs.array_random_update, true);
    else {
        fprintf(stderr, "Unknown program %s\n", prog);
        fprintf(stderr, "Available: array_lookup_only array_lookup_update array_update_only array_multiple_lookups array_sequential_update array_random_update\n");
        array_perf_bpf__destroy(skel);
        return 1;
    }

    if (array_perf_bpf__load(skel)) {
        fprintf(stderr, "Failed to load skeleton\n");
        array_perf_bpf__destroy(skel);
        return 1;
    }
    if (array_perf_bpf__attach(skel)) {
        fprintf(stderr, "Failed to attach programs\n");
        array_perf_bpf__destroy(skel);
        return 1;
    }

    printf("Loaded BPF program variant: %s\n", prog);
    printf("Press Ctrl+C to stop...\n");
    signal(SIGINT, on_sig); signal(SIGTERM, on_sig);
    while (!stop) sleep(1);

    array_perf_bpf__destroy(skel);
    return 0;
}
