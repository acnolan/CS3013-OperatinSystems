/*
 * phase1_module.c
 * Andrew Nolan - acnolan
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

//space to save the sys_call_table
unsigned long **sys_call_table;

//pointers to the sys calls we are going to intercept
asmlinkage long (*ref_sys_open)(const char __user *filename, int flags, umode_t mode);
asmlinkage long (*ref_sys_close)(unsigned int fd);
asmlinkage long (*ref_sys_cs3013_syscall1)(void);


//intercepts the system calls
asmlinkage long new_sys_open(const char __user *filename, int flags, umode_t mode) {
	unsigned int userId = current_uid().val;
	if(userId > 999){
		printk(KERN_INFO "User %u is opening file: %s\n", userId, filename);	
	}
	return ref_sys_open(filename, flags, mode);
}
asmlinkage long new_sys_close(unsigned int fd) {
	unsigned int userId = current_uid().val;
	if(userId > 999){
		printk(KERN_INFO "User %u is closing file descriptor: %u\n", userId, fd);	
	}
	return ref_sys_close(fd);
}
asmlinkage long new_sys_cs3013_syscall1(void) {
    printk(KERN_INFO "\"'Hello world?!' More like 'Goodbye, world!' EXTERMINATE!\" -- Dalek");
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
	ref_sys_open = (void *)sys_call_table[__NR_open];
	ref_sys_close = (void *)sys_call_table[__NR_close];
	ref_sys_cs3013_syscall1 = (void *)sys_call_table[__NR_cs3013_syscall1];
	/* Replace the existing system calls */
	disable_page_protection();
	sys_call_table[__NR_open] = (unsigned long *)new_sys_open;  
	sys_call_table[__NR_close] = (unsigned long *)new_sys_close;
	sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)new_sys_cs3013_syscall1;
	enable_page_protection();  
	/* And indicate the load was successful */
	printk(KERN_INFO "Loaded interceptors!\n");
	return 0;
}

static void __exit interceptor_end(void) {
	/* If we don't know what the syscall table is, don't bother. */
	if(!sys_call_table)
		return;  
	/* Revert all system calls to what they were before we began. */
	disable_page_protection();
	sys_call_table[__NR_open] = (unsigned long *)ref_sys_open;
	sys_call_table[__NR_close] = (unsigned long *)ref_sys_close;
	sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)ref_sys_cs3013_syscall1;
	enable_page_protection();
	printk(KERN_INFO "Unloaded interceptors!\n");
}

MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);
