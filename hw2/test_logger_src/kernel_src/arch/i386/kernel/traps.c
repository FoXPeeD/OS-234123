
/*
void show_trace(unsigned long * stack) ...
*/

/* hw2 ctx_log */
unsigned int store_trace(unsigned long * stack, func_call_trace_info_t *calls_array, unsigned int max_calls)
{
	unsigned int calls_count;
	unsigned long addr;

	if (!stack)
		stack = (unsigned long*)&stack;

	calls_count = 0;
	while (((long) stack & (THREAD_SIZE-1)) != 0) {
		addr = *stack++;
		if (kernel_text_address(addr)) {
            calls_array[calls_count].addr = addr;
            calls_array[calls_count].stack = stack-1;
            if (++calls_count >= max_calls) return calls_count;
		}
	}
	return calls_count;
}

/*
void show_trace_task(struct task_struct *tsk)
*/
