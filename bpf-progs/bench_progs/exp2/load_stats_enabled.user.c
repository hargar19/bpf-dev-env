#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <spawn.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

extern char **environ;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Not enough args\n");
        printf("Expected: ./load.user bpf_file\n");
        return -1;
    }

    char * bpf_path = argv[1];
    char * prog_name = "trigger_syscall_prog";
	
	pid_t pid;
	char * const argv_new[] = { "taskset", "-c", "1", "./bench.user", "30", NULL};

	 int stats_fd = bpf_enable_stats(BPF_STATS_RUN_TIME);
	 if (stats_fd < 0) {
		printf("Failed to enable the stats with err %d\n", stats_fd);
		return 0;
     }
    struct bpf_object * prog = bpf_object__open(bpf_path);
    
    if (bpf_object__load(prog)) {
        printf("Failed");
		return 0;
    }

    struct bpf_program * program = bpf_object__find_program_by_name(prog, prog_name);

    if (program == NULL) {
        printf("Shared 1 failed\n");
		bpf_object__close(prog);
		return 0;
    }

    struct bpf_link *link = bpf_program__attach(program);
	if (libbpf_get_error(link)) {
		printf("Attachement failed\n");
		goto cleanup;
	}

	int rc = posix_spawnp(&pid, "taskset", NULL, NULL, argv_new, environ);
	if (rc != 0) {
		fprintf(stderr, "posix_spawnp(taskset) failed: %s\n", strerror(rc));
		return -1;
	}
	waitpid(pid, NULL, 0);


cleanup:
	bpf_link__destroy(link);
	bpf_object__close(prog);
    return 0;
}
