
/*
extern struct task_struct *child_reaper;
*/
extern void set_last_needresched_reason(task_t* p, ctx_reason_t reason);  /* hw2 ctx_log */
/*
int getrusage(struct task_struct *, int, struct rusage *);
*/





NORET_TYPE void do_exit(long code)
{
	/* ... */
	/*exit_notify();*/
    set_last_needresched_reason(current, CTX_DO_EXIT);  /* hw2 ctx_log */
	/*schedule();*/
	/* ... */
}





asmlinkage long sys_wait4(pid_t pid,unsigned int * stat_addr, int options, struct rusage * ru)
{
	/* ... */
	/*if (flag) {
		retval = 0;
		if (options & WNOHANG)
			goto end_wait4;
		retval = -ERESTARTSYS;
		if (signal_pending(current))
			goto end_wait4;*/
        set_last_needresched_reason(current, CTX_WAIT4);  /* hw2 ctx_log */
		/*schedule();
		goto repeat;
	}
	retval = -ECHILD;*/
	/* ... */
}
