# Makefile for BPF array benchmarks (uses common infrastructure)

CLANG := clang
# Relative paths to kernel source tree (adjusted for array_value_size_tests subdirectory)
KERNEL_ROOT := ../../../../../../linux
LIBBPF_DIR := $(KERNEL_ROOT)/tools/lib/bpf
BPFTOOL_DIR := $(KERNEL_ROOT)/tools/bpf/bpftool
BPFTOOL := $(BPFTOOL_DIR)/bpftool

# Common directory with shared files
COMMON_DIR := ../common

INCLUDE := -I$(KERNEL_ROOT)/usr/include -I$(KERNEL_ROOT)/tools/lib -I$(LIBBPF_DIR)
BPF_CFLAGS := -target bpf -g -O2 -Wall -c $(INCLUDE)
USER_CFLAGS := -g -O2 -Wall $(INCLUDE)

# Targets
BPF_OBJ := array_bench.bpf.o
SKEL_H := array_bench.skel.h
LOADER := bench_loader
RUNNER := bench_run
COUNTER_READER := read_counter

all: $(BPF_OBJ) $(SKEL_H) $(LOADER) $(RUNNER) $(COUNTER_READER)

# Compile BPF program (experiment-specific)
$(BPF_OBJ): array_bench.kern.c
	$(CLANG) $(BPF_CFLAGS) -o $@ $<

# Generate skeleton
$(SKEL_H): $(BPF_OBJ)
	$(MAKE) -C $(KERNEL_ROOT)/tools/bpf bpftool
	$(BPFTOOL) gen skeleton $< > $@

# Compile loader (from common directory)
$(LOADER): $(COMMON_DIR)/bench_loader.c $(SKEL_H)
	$(MAKE) -C $(LIBBPF_DIR)
	gcc $(USER_CFLAGS) -I. $< $(LIBBPF_DIR)/libbpf.a -lelf -lz -o $@

# Compile runner (from common directory)
$(RUNNER): $(COMMON_DIR)/bench_run.c
	gcc $(USER_CFLAGS) $< -o $@

# Compile counter reader (from common directory)
$(COUNTER_READER): $(COMMON_DIR)/read_counter.c
	$(MAKE) -C $(LIBBPF_DIR)
	gcc $(USER_CFLAGS) $< $(LIBBPF_DIR)/libbpf.a -lelf -lz -o $@

clean:
	rm -f $(BPF_OBJ) $(SKEL_H) $(LOADER) $(RUNNER) $(COUNTER_READER)

.PHONY: all clean
