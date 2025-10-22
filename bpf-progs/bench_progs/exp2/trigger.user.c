#include <stdio.h>
#include <unistd.h>

#define _NR_BPFPROF 470

int main(void)
{
	syscall(_NR_BPFPROF);
	return 0;
}
