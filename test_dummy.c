#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#ifndef __NR_dummy
#define __NR_dummy 470
#endif

int main(void) {
    printf("Testing dummy syscall (number %d)...\n", __NR_dummy);
    
    long ret = syscall(__NR_dummy);
    
    if (ret == -1) {
        perror("dummy syscall failed");
        return 1;
    }
    
    printf("SUCCESS: dummy syscall returned %ld (expected 0)\n", ret);
    
    // Test multiple calls
    printf("Testing multiple calls:\n");
    for (int i = 0; i < 5; i++) {
        ret = syscall(__NR_dummy);
        printf("Call %d: returned %ld\n", i+1, ret);
    }
    
    return 0;
}