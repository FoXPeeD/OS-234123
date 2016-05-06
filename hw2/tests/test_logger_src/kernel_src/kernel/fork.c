
int do_fork(unsigned long clone_flags, unsigned long stack_start,
	    struct pt_regs *regs, unsigned long stack_size)
{
	/* ... */
	/*p->state = TASK_UNINTERRUPTIBLE;*/

    p->last_needresched_reason = CTX_DEFAULT;  /* hw2 ctx_log */
    
	/*copy_flags(clone_flags, p);*/
	/* ... */
}
