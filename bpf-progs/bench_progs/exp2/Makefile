SOURCES := $(wildcard *.kern.c)
FILES := $(SOURCES:.c=.o)
USER_SRC := $(wildcard *.user.c)
USER_SKEL := $(SOURCES:.c=.skel.h)
USER_LSKEL := $(SOURCES:.c=.lskel.h)
USER := $(USER_SRC:.c=)
BPF-CLANG := clang
BPF_CLANG_CFLAGS := -target bpf -g -Wall -O2 -c
INCLUDE := -I /linux/usr/include/ -I /linux/tools/lib/
USER-CFLAGS := -g -I /linux/usr/include -I /linux/tools/lib/ -L /linux/tools/lib/bpf/
BPFTOOL := /linux/tools/bpf/bpftool/bpftool

all: vmlinux-header $(FILES) $(USER_SKEL) $(USER_LSKEL) $(USER)
	
$(FILES) : %.o : %.c
	$(BPF-CLANG) $(INCLUDE) $(BPF_CLANG_CFLAGS) -o $@ $<

$(USER) : % : %.c
	gcc $(USER-CFLAGS) $< -lbpf -o $@

$(USER_SKEL) : %.skel.h : %.o
	$(BPFTOOL) gen skeleton $<  > $@

$(USER_LSKEL) : %.lskel.h : %.o
	$(BPFTOOL) gen skeleton -L $<  > $@

vmlinux-header:
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

.PHONY : clean

clean:
	rm -f $(FILES) $(USER) $(USER_SKEL) $(USER_LSKEL) vmlinux.h
