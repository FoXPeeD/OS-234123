
/*
 * Scheduling policies
    ...
    #define SCHED_SHORT		5
 */

/**
    hw2 : ctx_log
    If you add a new elements here - add it also to the enum in ctx_log_wrapper.h
**/
typedef enum ctx_reason {
    CTX_DEFAULT,
    CTX_SCHEDULER_TICK, 
    CTX_SET_USER_NICE, 
    CTX_TRY_TO_WAKE_UP,
    CTX_DO_FORK, 
    CTX_YIELD, 
    CTX_SLEEP_ON, 
    CTX_SETSCHEDULER, 
    CTX_DO_EXIT, /*(AT EXIT FILE)*/
    CTX_WAIT4, /*(AT EXIT FILE)*/
    CTX_KICK_IF_RUNNING,
    CTX_SCHEDULER_LOOP,
    CTX_LOAD_BALANCE,
    CTX_CPU_IDLE,
    CTX_SCHEDULE_TIMEOUT
} ctx_reason_t;

/* hw2 ctx_log*/
#define FTRACE_SYMBOL_STR_MAX_SIZE 40
typedef struct func_call_trace_info {
    unsigned long addr, stack;
    char symbol[FTRACE_SYMBOL_STR_MAX_SIZE];
} func_call_trace_info_t;

/*
struct sched_param ...
*/




struct task_struct {
    
    /* .... */
    
/* hw2: ctx_log */
    ctx_reason_t last_needresched_reason;
    int last_needresched_info;
};




#define INIT_TASK(tsk)	\
{									\

    /* ... */
    
    last_needresched_reason:        CTX_DEFAULT, \
    last_needresched_info:      0, \
}

