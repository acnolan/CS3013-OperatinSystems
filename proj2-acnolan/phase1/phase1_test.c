/*
 * phase1_test.c
 * Andrew Nolan - acnolan
 */

#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>

// These values MUST match the syscall_32.tbl modifications:
#define __NR_cs3013_syscall1 377

/*
 * testCall1
 * calls the newly interceptred cs3013_syscall1 function
 */
long testCall1 (void) {
        return (long) syscall(__NR_cs3013_syscall1);
}

/*
 *main
 *executes the system call
 */
int main () {
        printf("The return values of the system calls are:\n");
        printf("\tcs3013_syscall1: %ld\n", testCall1());
	printf("\tThe message produced by syscall1 can be seen in /var/log/syslog\n");
        printf("\tLook in /var/log/syslog to see the affects of the open and close interceptors\n");
        return 0;
}
