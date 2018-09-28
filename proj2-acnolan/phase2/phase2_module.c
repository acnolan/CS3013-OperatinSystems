/*
 * phase2_module.c
 * Andrew Nolan - acnolan
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <asm-generic/current.h>
#include <asm-generic/cputime.h>

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

//space to save the sys_call_table
unsigned long **sys_call_table;

//pointer to the sys call we are going to intercept
asmlinkage long (*ref_sys_cs3013_syscall2)(void);

//intercepts the system call
asmlinkage long new_sys_cs3013_syscall2(struct processinfo *info) {
	struct processinfo pinfo; //make a struct to hold the process info we are getting
	struct task_struct *task = current; //initiate the task struct for the current process
	struct list_head *head; //not really sure what it is but I need it for list_for_each()
	pid_t youngestChild = -1; //place to store the youngest child pid while we loop or -1 if none
	long long youngestChildStart = -1;
	pid_t youngerSibling = -1; //place to store younger sibling pid while we loop or -1 if none
	long long youngerSiblingStart=-1;
	pid_t olderSibling = -1; //place to store older sibling pid while we loop or -1 if none
	long long olderSiblingStart=-1;
	pinfo.state = task->state; //get the state
	pinfo.pid = task->pid; //get the pid
	pinfo.parent_pid = task->parent->pid; //get the parent pid
	pinfo.cstime = 0;
	pinfo.cutime = 0;
	//loop through the children and find the youngest one
	list_for_each(head, &(task->children)){
		struct task_struct* childTask = list_entry(head, struct task_struct, sibling);
		//long long taskTime = timespec_to_ns(childTask.start_time);
		long long taskTime = (childTask->start_time);
		pinfo.cstime += cputime_to_usecs(childTask->stime);
		pinfo.cutime += cputime_to_usecs(childTask->utime);
		if(taskTime>youngestChildStart || youngestChildStart==-1){
			youngestChild=childTask->pid;
			youngestChildStart = taskTime;
		}
	}
	list_for_each(head, &task->sibling){
		struct task_struct* siblingTask = list_entry(head, struct task_struct, sibling);
		//long long taskTime = timespec_to_ns(&siblingTask.start_time);
		long long taskTime = (siblingTask->start_time);
		if(taskTime > pinfo.start_time && (taskTime - pinfo.start_time < youngerSiblingStart || youngerSiblingStart == -1)){
			youngerSibling = siblingTask->pid;
			youngerSiblingStart = taskTime - pinfo.start_time;
		}
		if(taskTime < pinfo.start_time && (pinfo.start_time - taskTime > olderSiblingStart || olderSiblingStart == -1)){
			olderSibling = siblingTask->pid;
			olderSiblingStart = taskTime - pinfo. start_time;
		}
	}
	pinfo.youngest_child = youngestChild;
	pinfo.younger_sibling = youngerSibling;
	pinfo.older_sibling = olderSibling;
	pinfo.uid = current_uid().val;
	//pinfo.start_time = timespec_to_ns(task->start_time);
	pinfo.start_time = (task->start_time);
	pinfo.user_time = cputime_to_usecs(task->utime);
	pinfo.sys_time = cputime_to_usecs(task->stime);
	if(copy_to_user(info, &pinfo, sizeof pinfo)){
		return EFAULT;
	}
	return 0;
}

//find the sys call in the table
static unsigned long **find_sys_call_table(void) {
	unsigned long int offset = PAGE_OFFSET;
	unsigned long **sct;  
	while (offset < ULLONG_MAX) {
		sct = (unsigned long **)offset;
		if (sct[__NR_close] == (unsigned long *) sys_close) {
			printk(KERN_INFO "Interceptor: Found syscall table at address: 0x%02lX\n", (unsigned long) sct);
			return sct;
		} 
    	offset += sizeof(void *);
	}  
	return NULL;
}

//disable page protection so we can edit the table
static void disable_page_protection(void) {
	write_cr0 (read_cr0 () & (~ 0x10000));
}

//reenable page protection
static void enable_page_protection(void) {
	write_cr0 (read_cr0 () | 0x10000);
}

static int __init interceptor_start(void) {
	/* Find the system call table */
	if(!(sys_call_table = find_sys_call_table())) {
		/* Well, that didn't work. Cancel the module loading step. */
		return -1;
	}  
	/* Store a copy of all the existing functions */
	ref_sys_cs3013_syscall2 = (void *)sys_call_table[__NR_cs3013_syscall2];
	/* Replace the existing system calls */
	disable_page_protection();
	sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)new_sys_cs3013_syscall2;  
	enable_page_protection();  
	/* And indicate the load was successful */
	printk(KERN_INFO "Loaded phase 2 interceptor!\n");
	return 0;
}

static void __exit interceptor_end(void) {
	/* If we don't know what the syscall table is, don't bother. */
	if(!sys_call_table)
		return;  
	/* Revert all system calls to what they were before we began. */
	disable_page_protection();
	sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)ref_sys_cs3013_syscall2;
	enable_page_protection();
	printk(KERN_INFO "Unloaded phase 2 interceptor!\n");
}

MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);
