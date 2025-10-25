SOURCES := $(wildcard *.kern.c)
BPF_OBJS := $(SOURCES:.kern.c=.bpf.o)
SKELS := $(SOURCES:.kern.c=.skel.h)
USER_SRC := $(wildcard *.user.c)
USERS := $(USER_SRC:.user.c=)
CLANG := clang
BPFTOOL := /linux/tools/bpf/bpftool/bpftool
LIBBPF_DIR := /linux/tools/lib/bpf

INCLUDE := -I/linux/usr/include/ -I/linux/tools/lib/ -I$(LIBBPF_DIR)
BPF_CFLAGS := -target bpf -g -O2 -Wall -c $(INCLUDE)
USER_CFLAGS := -g -Wall $(INCLUDE)

all: $(BPF_OBJS) $(SKELS) load_array bench

%.bpf.o: %.kern.c
	$(CLANG) $(BPF_CFLAGS) -o $@ $<

%.skel.h: %.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

load_array: load_array.user.c $(SKELS)
	gcc $(USER_CFLAGS) $< $(LIBBPF_DIR)/libbpf.a -lelf -lz -o $@

# bench does not require libbpf linkage; it only issues the test syscall
bench: bench.user.c
	gcc $(USER_CFLAGS) $< -o $@

clean:
	rm -f $(BPF_OBJS) $(SKELS) load_array bench

.PHONY: all clean
