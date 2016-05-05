/* ... */

    /* hw2 ctx_log for testing */
    .long SYMBOL_NAME(sys_get_last_ctx_id) /* 246 */
    .long SYMBOL_NAME(sys_get_ctx_log)
    .long SYMBOL_NAME(sys_flush_ctx_log)
    .long SYMBOL_NAME(sys_wait_for_completion_of_debug_work)
    .long SYMBOL_NAME(sys_complete_debug_work)

/*
	.rept NR_syscalls-(.-sys_call_table)/4
	...
*/