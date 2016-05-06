#ifndef CTX_LOG_WRAPPERS_H
#define CTX_LOG_WRAPPERS_H

#include <errno.h>
#include <stdio.h>
#include <string.h>

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

static char* arr_ctx_reason_str[] = {
    "DEFAULT(?)",
    "SCHEDULER_TICK", 
    "SET_USER_NICE", 
    "TRY_TO_WAKE_UP",
    "DO_FORK", 
    "YIELD", 
    "SLEEP_ON", 
    "SETSCHEDULER", 
    "DO_EXIT",
    "WAIT4",
    "KICK_IF_RUNNING",
    "SCHEDULER_LOOP",
    "LOAD_BALANCE",
    "CPU_IDLE",
    "SCHEDULE_TIMEOUT"
};
#define CTX_REASON_STR(reason) (arr_ctx_reason_str[reason])

typedef enum ctx_prio_array {
    CTX_ACTIVE,
    CTX_EXPIRED,
    CTX_SHORTS,
    CTX_OVERDUE_SHORTS,
    CTX_NO_PRIO_ARRAY,
    CTX_OTHER_RQ
} ctx_prio_array_t;

static char* arr_ctx_prio_array[] = {
    "active",
    "expired", 
    "shorts", 
    "overdue",
    "OutOfRQ", 
    "UNKNOWN"
};
#define CTX_PRIO_ARRAY_STR(array) (arr_ctx_prio_array[array])

#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_ZOMBIE		4
#define TASK_STOPPED		8

/*
 * Scheduling policies
 */
#define SCHED_OTHER		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_SHORT		5

#define NR_CTX_LOG_TRACE_MAX_CALLS 9
#define FTRACE_SYMBOL_STR_MAX_SIZE 40

typedef struct func_call_trace_info {
    unsigned long addr, stack;
    char symbol[FTRACE_SYMBOL_STR_MAX_SIZE];
} func_call_trace_info_t;

typedef struct ctx_idx {
    unsigned long id;   // used for indeces aritmetics (substracting after overflow)
    size_t table_idx;   // modulus log table size.
    unsigned long table_cycle;
} ctx_idx_t;

#define NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS 5
#define CTX_LOG_PRIO_QUEUE_SIZE(queue) (min((queue)->nr_procs, NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS))
#define MAX_PRIO 140
#define MAX_RT_PRIO 100
typedef struct ctx_log_prio_queue {
    unsigned int nr_procs;  // actual queue size. the `procs` array might has less entries (up to NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS)
    int prio;
    pid_t procs[NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS];
    pid_t last, before_last;
} ctx_log_prio_queue_t;

typedef struct ctx_log_rec_task_info {
    int pid; // pid_t
    int ppid; // pid_t
    long state;
    int prio;
    int static_prio;
    ctx_prio_array_t prio_array;
    unsigned long policy;
    unsigned int is_interactive;
    unsigned int is_during_overdue_period;
    unsigned int is_overdue_forever;
    unsigned int remaining_timeslice;
    unsigned int is_idle;
    unsigned int is_first_timeslice;
    
    int remaining_cooloff_cycles;
    int initial_cooloff_cycles;
    int requested_time;
    int timeslice_for_next_cooloff;
    
    ctx_log_prio_queue_t current_queue;
    
} ctx_log_rec_task_info_t;

#define CTX_LOG_MAX_SIZE 5000

typedef struct ctx_log_record {
    ctx_idx_t ctx_id;
    unsigned long current_jiffies;
    ctx_reason_t reason;
    int needresched_info;
    ctx_log_rec_task_info_t  prev_info;
    ctx_log_rec_task_info_t  next_info;
    
    func_call_trace_info_t calls_trace[NR_CTX_LOG_TRACE_MAX_CALLS];
    unsigned int nr_calls_trace;
    
    ctx_log_prio_queue_t best_prio_queues[4]; // CTX_ACTIVE, CTX_EXPIRED, CTX_SHORTS, CTX_OVERDUE_SHORTS
    int nr_active[4]; // CTX_ACTIVE, CTX_EXPIRED, CTX_SHORTS, CTX_OVERDUE_SHORTS
    
} ctx_log_record_t;

int write_ctx_log_to_file(ctx_log_record_t log[], int nr_records, const char* filename) {
    if (!log || !filename) return -1;
    FILE * file= fopen(filename, "wb");
    if (!file) return -1;
    if (fwrite((void*)(&nr_records), sizeof(int), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    if (fwrite((void*)log, sizeof(ctx_log_record_t), nr_records, file) != nr_records) {
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

ctx_log_record_t* read_ctx_log_from_file(const char* filename, int *nr_records) {
    if (!filename || !nr_records) return NULL;
    FILE * file= fopen(filename, "rb");
    ctx_log_record_t* log = NULL;
    if (!file) return NULL;
    if (fread((void*)(nr_records), sizeof(int), 1, file) != 1) {
        fclose(file);
        return NULL;
    }
    log = (ctx_log_record_t*)malloc((*nr_records) * sizeof(ctx_log_record_t));
    if (!log) {
        fclose(file);
        return NULL;
    }
    if (fread((void*)log, sizeof(ctx_log_record_t), *nr_records, file) != *nr_records) {
        free(log);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    return log;
}

int get_last_ctx_id(ctx_idx_t* idx) {
    unsigned int res;
    __asm__
    (
        "int $0x80;"
        : "=a" (res)
        : "0" (246), "b" (idx)
        : "memory"
    );
    if (res>= (unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return (int) res;
}

int get_ctx_log(ctx_log_record_t* user_table, int nr_max_records) {
    unsigned int res;
    __asm__
    (
        "int $0x80;"
        : "=a" (res)
        : "0" (247) ,"b" (user_table), "c" (nr_max_records)
        : "memory"
    );
    if (res>= (unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return (int) res;
}

int flush_ctx_log() {
    unsigned int res;
    __asm__
    (
        "int $0x80;"
        : "=a" (res)
        : "0" (248)
        : "memory"
    );
    if (res>= (unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return (int) res;
}

int wait_for_completion_of_debug_work() {
    unsigned int res;
    __asm__
    (
        "int $0x80;"
        : "=a" (res)
        : "0" (249)
        : "memory"
    );
    if (res>= (unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return (int) res;
}

int complete_debug_work() {
    unsigned int res;
    __asm__
    (
        "int $0x80;"
        : "=a" (res)
        : "0" (250)
        : "memory"
    );
    if (res>= (unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return (int) res;
}

#endif // CTX_LOG_WRAPPERS_H
