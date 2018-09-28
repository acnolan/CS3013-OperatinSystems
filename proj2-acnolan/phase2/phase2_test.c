/*
 * phase2_test.c
 * Andrew Nolan - acnolan
 */

#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// These values MUST match the syscall_32.tbl modifications:
#define __NR_cs3013_syscall2 378

//the struct to store process info
struct processinfo{
	long state;		//current state of the process
	pid_t pid;		//process ID of this process
	pid_t parent_pid;	//process ID of parent
	pid_t youngest_child;	//process ID of youngest child
	pid_t younger_sibling;	//pid of next younger sibling
	pid_t older_sibling;	//pid of next old sibling
	uid_t uid;		//user ID of process owner
	long long start_time;	//process start time in nanoseconds since boot time
	long long user_time;	//CPU time in user mode (microseconds)
	long long sys_time;	//CPU time in system mode (microseconds)
	long long cutime;	//user time of children (microseconds)
	long long cstime;	//system time of children (microseconds)
};

/*
 * testCall2
 * calls the system call cs3013_syscall2
 * @param info - the pointer to the processinfo struct we are going to copy data to
 */
long testCall2(struct processinfo *info) {
        return (long) syscall(__NR_cs3013_syscall2, info);
}

/*
 * printStats
 * prints out the statistics of the process
 * @param pinfo - a processinfo struct containing the info to print
 */
void printStats(struct processinfo pinfo){
        printf("\tState: %lu\n", pinfo.state );
	printf("\tPid: %d\n", pinfo.pid);
	printf("\tParent Pid: %d\n", pinfo.parent_pid);
	printf("\tYoungest Child Pid: %d\n", pinfo.youngest_child);
	printf("\tYounger Sibling Pid: %d\n", pinfo.younger_sibling);
	printf("\tOlder Sibling Pid: %d\n", pinfo.older_sibling);
	printf("\tUid: %u\n", pinfo.uid);
	printf("\tStart Time: %lld nanoseconds\n", pinfo.start_time);
	printf("\tUser Time: %lld microseconds\n", pinfo.user_time);
	printf("\tSystem Time: %lld microseconds\n", pinfo.sys_time);
	printf("\tChildren User Time: %lld microseconds\n", pinfo.cutime);
	printf("\tChildren System Time: %lld microseconds\n", pinfo.cstime);
}

/*
 * main
 * runs the test code
 */
int main () {
	struct processinfo pinfo1;
	struct processinfo pinfo3;
	int pid;
	int busyWorkCounter;
	//do some busy work waiting to put time into the process (hopefully)
	for(int j=0;j<10000000;j++){
		busyWorkCounter++;
	}
	testCall2(&pinfo1);
	printf("The process info returned by cs3013_syscall2 with no children is:\n");
	printStats(pinfo1);
	
	busyWorkCounter=0;
	struct processinfo pinfo2;
	//do some busy work waiting to put time into the process (hopefully)
	for(int j=0;j<10000000;j++){
		busyWorkCounter++;
	}
	//make some children for the process so we can test to make sure that part works
	for(int i=0;i<3;i++){
		pid=fork();
		if(pid==0){
			//do some busy work waiting loop to put time into the children (hopefully)
			for(int j=0;j<10000000;j++){
				busyWorkCounter++;
			}
			return 0;
		}
	}
	testCall2(&pinfo2);
	printf("\nThe process info returned by cs3013_syscall2 with children is:\n");
	printStats(pinfo2);    	
		
        return 0;
}

