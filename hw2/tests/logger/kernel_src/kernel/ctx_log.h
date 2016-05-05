#ifndef CTX_LOG_H
#define CTX_LOG_H

/******** CTX LOG ********/

#define NR_CTX_LOG_TABLE_RECORDS 5000
#define NR_CTX_LOG_TRACE_MAX_CALLS 9
#define NEXT_CXT_LOG_IDX(table_idx) (((table_idx)+1) % NR_CTX_LOG_TABLE_RECORDS)
#define PREV_CXT_LOG_IDX(table_idx) ((table_idx) == 0 ? NR_CTX_LOG_TABLE_RECORDS-1 : ((table_idx)-1))
#define CXT_LOG_FULLLY_WRITTEN(log) ((log)->biggest_written_idx == NR_CTX_LOG_TABLE_RECORDS-1)
#define CTX_LOG_VALID_IDX(log, table_idx) (CXT_LOG_FULLLY_WRITTEN(log) || (table_idx) <= (log)->biggest_written_idx)
#define CTX_LOG_NR_RECORDS(log) (CXT_LOG_FULLLY_WRITTEN(log) ? NR_CTX_LOG_TABLE_RECORDS : ((log)->curr_idx.table_idx + 1))
#define cxt_log_for_each(log, record, idx) \
	for (idx = (log)->curr_idx.table_idx, record = NULL; \
        ((record == NULL && (record = ((log)->table + (log)->curr_idx.table_idx))) || idx != ((log)->curr_idx.table_idx)) && (CTX_LOG_VALID_IDX(log, idx)); \
        idx = PREV_CXT_LOG_IDX(idx), record = ((log)->table + idx))
#define cpu_ctx_log(cpu)    (ctx_logs + (cpu))
#define this_ctx_log()      cpu_ctx_log(smp_processor_id())
#define task_ctx_log(p)     cpu_ctx_log((p)->cpu)
#define IS_CTX_RECORD_SET(rec) ((rec)->pid != 0)
#define IS_TASK_IDLE(p) (task_rq(p)->idle == (p))
#define CTX_TASK_PRIO_ARRAY(p) \
    ((p)->array == NULL \
    ? CTX_NO_PRIO_ARRAY \
    : ((p)->array == task_rq(p)->active \
    ? CTX_ACTIVE \
    : ((p)->array == task_rq(p)->expired \
    ? CTX_EXPIRED \
    : ((p)->array == task_rq(p)->shorts \
    ? CTX_SHORTS \
    : ((p)->array == task_rq(p)->overdue_shorts \
    ? CTX_OVERDUE_SHORTS \
    : CTX_OTHER_RQ )))))
#define KERNEL_SYMBOL_SIZE 256


typedef enum ctx_prio_array {
    CTX_ACTIVE,
    CTX_EXPIRED,
    CTX_SHORTS,
    CTX_OVERDUE_SHORTS,
    CTX_NO_PRIO_ARRAY,
    CTX_OTHER_RQ
} ctx_prio_array_t;

typedef struct ctx_idx {
    unsigned long id;  // used for indeces aritmetics (substracting after overflow)
    size_t table_idx;   // modulus log table size.
    unsigned long table_cycle;
} ctx_idx_t;

#define NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS 5
#define CTX_LOG_PRIO_QUEUE_SIZE(queue) (min((queue)->nr_procs, NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS))
typedef struct ctx_log_prio_queue {
    unsigned int nr_procs;  // actual queue size. the `procs` array might has less entries (up to NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS)
    int prio;
    pid_t procs[NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS];
    pid_t last, before_last;
} ctx_log_prio_queue_t;

typedef struct ctx_log_rec_task_info {
    pid_t pid;
    pid_t ppid;
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
    /**
        If you add a new field here - add it also to the struct in ctx_log_wrapper.h
    **/
} ctx_log_rec_task_info_t;

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
    /**
        If you add a new field here - add it also to the struct in ctx_log_wrapper.h
    **/
} ctx_log_record_t;

typedef struct ctx_log {
    ctx_log_record_t table[NR_CTX_LOG_TABLE_RECORDS];
    ctx_idx_t curr_idx;
    size_t biggest_written_idx;
} ctx_log_t;

static struct ctx_log ctx_logs[NR_CPUS];

static void ctx_logs_init() {
    unsigned int i;
    for (i = 0; i < NR_CPUS; i++) {
        cpu_ctx_log(i)->curr_idx.id = 0;
        cpu_ctx_log(i)->curr_idx.table_idx = 0;
        cpu_ctx_log(i)->curr_idx.table_cycle = 0;
        cpu_ctx_log(i)->biggest_written_idx = 0;
    }
}

inline static void assign_prio_queue(ctx_log_prio_queue_t* queue_log, list_t* prio_queue, int prio) {
    unsigned int count = 0;
    list_t* pos;
    task_t* task;
    
    queue_log->prio = prio;
    if (prio_queue == NULL || prio == MAX_PRIO) {
        queue_log->nr_procs = 0;
        return;
    }
    queue_log->nr_procs = list_length(prio_queue);
    
    list_for_each(pos, prio_queue) {
        task = list_entry(pos, task_t, run_list);
        queue_log->procs[count] = task->pid;
        count++;
        if (count >= NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS) break;
    }
    
    queue_log->last = queue_log->before_last = 0;
    pos = prio_queue->prev;
    if (pos != prio_queue) {
        task = list_entry(pos, task_t, run_list);
        queue_log->last = task->pid;
        pos = pos->prev;
        if (pos != prio_queue) {
            task = list_entry(pos, task_t, run_list);
            queue_log->before_last = task->pid;
        }
    }
}

inline static void assign_best_prio_queue(ctx_log_prio_queue_t* queue_log, prio_array_t* array) {
    int idx;
    list_t* prio_queue;
    
    idx = sched_find_first_bit(array->bitmap);
    prio_queue = (idx == MAX_PRIO) ? NULL : (array->queue + idx);
    assign_prio_queue(queue_log, prio_queue, idx);
}

inline static void set_ctx_log_rec_task_info(ctx_log_rec_task_info_t* info, task_t* p) {
    if (!p) return; // the system craches without this chech. probabbly for first proc.
    
    info->pid = p->pid;
    info->ppid = p->p_pptr ? p->p_pptr->pid : 0;
    info->state = p->state;
    info->prio = p->prio;
    info->static_prio = p->static_prio;
    info->prio_array = CTX_TASK_PRIO_ARRAY(p);
    info->policy = p->policy;
    info->is_interactive = TASK_INTERACTIVE(p);
    info->is_during_overdue_period = TASK_SHORT_OVERDUE(p);
    info->is_overdue_forever = TASK_OVERDUE_FOREVER(p);
    info->remaining_timeslice = p->time_slice;
    info->is_idle = IS_TASK_IDLE(p);
    info->is_first_timeslice = p->first_time_slice;
    
    info->remaining_cooloff_cycles = p->remaining_cooloff_cycles;
    info->initial_cooloff_cycles = p->initial_cooloff_cycles;
    info->requested_time = p->requested_time;
    info->timeslice_for_next_cooloff = p->timeslice_for_next_cooloff;
    
    assign_prio_queue(&info->current_queue, ((p->array) ? TASK_QUEUE(p, p->array) : NULL), TASK_PRIO_IN_PRIO_ARR(p));
}


/*
    Assuming the ctx_log is locked (means the rq is locked).
    When calling from context_switch the rq is indeed locked.
*/
extern unsigned int store_trace(unsigned long * stack, unsigned long calls_array[][2], unsigned int max_calls); // traps.c
inline static void log_ctx(task_t *prev, task_t *next) {
    runqueue_t* rq;
    ctx_log_t* log;
    
    kassert(prev); // might fail for first proc
    kassert(next);
    
    if (!prev || !next) return;
    
    rq = task_rq(next); // prev might be out of queue here (went to wait)
    log = task_ctx_log(prev);
    log->curr_idx.id++;
    log->curr_idx.table_idx = NEXT_CXT_LOG_IDX(log->curr_idx.table_idx);
    if (log->curr_idx.table_idx == 0) log->curr_idx.table_cycle++;
    ctx_log_record_t* record = (log->table + log->curr_idx.table_idx);
    
    record->ctx_id.id = log->curr_idx.id;
    record->ctx_id.table_idx = log->curr_idx.table_idx;
    record->ctx_id.table_cycle = log->curr_idx.table_cycle;
    record->current_jiffies = jiffies;
    set_ctx_log_rec_task_info(&record->prev_info, prev);
    set_ctx_log_rec_task_info(&record->next_info, next);
    
    if (log->curr_idx.table_idx > log->biggest_written_idx) {
        log->biggest_written_idx = log->curr_idx.table_idx;
    }
    
    record->reason = prev->last_needresched_reason;
    record->needresched_info = prev->last_needresched_info;
    prev->last_needresched_reason = CTX_DEFAULT;
    
    /* save best prio queues for each array */
    kassert(rq);    // next proc to run might be in a rq.
    if (rq) {       // to avoid disaster if assumptions do not take place.
        assign_best_prio_queue(record->best_prio_queues+0, rq->active);
        assign_best_prio_queue(record->best_prio_queues+1, rq->expired);
        assign_best_prio_queue(record->best_prio_queues+2, rq->shorts);
        assign_best_prio_queue(record->best_prio_queues+3, rq->overdue_shorts);
        
        record->nr_active[0] = rq->active->nr_active;
        record->nr_active[1] = rq->expired->nr_active;
        record->nr_active[2] = rq->shorts->nr_active;
        record->nr_active[3] = rq->overdue_shorts->nr_active;
    }
    
    /* save calls trace */
    int esp;
    __asm__("movl %%esp,%0" : "=r"(esp));
    record->nr_calls_trace = store_trace(esp, record->calls_trace, NR_CTX_LOG_TRACE_MAX_CALLS);
    
    if (record->reason == CTX_YIELD) {
        // strange: when printing there are many YIELD ctx. if not printing there are no YIELDs at all.
        //printk("log_ctx YIELD!! PID: %d\n", prev->pid);
    }
}

void set_last_needresched_reason_with_info(task_t* p, ctx_reason_t reason, int info) {
    kassert(p == current);
    p->last_needresched_reason = reason;
    p->last_needresched_info = info;
}

void set_last_needresched_reason(task_t* p, ctx_reason_t reason) {
    set_last_needresched_reason_with_info(p, reason, 0);
}

void set_last_needresched_reason_if_default(task_t* p, ctx_reason_t reason) {
    if (p->last_needresched_reason == CTX_DEFAULT) {
        set_last_needresched_reason(p, reason);
    }
}


asmlinkage int sys_get_last_ctx_id(ctx_idx_t* idx) {
    int retval = 0;
    runqueue_t *rq = this_rq_lock();  // used to lock the ctx_log
    
    if (idx == NULL) {
        retval = -EINVAL;
        goto out_unlock;
    }
    if (copy_to_user(idx, &this_ctx_log()->curr_idx, sizeof(ctx_idx_t))) {
        retval = -EFAULT;
        goto out_unlock;
    }
    
out_unlock:
    spin_unlock(&rq->lock);
	return retval;
}

asmlinkage int sys_get_ctx_log(ctx_log_record_t* user_table, int nr_max_records) {
    int retval = 0;
    runqueue_t *rq = this_rq_lock();  // used to lock the ctx_log
    
    ctx_log_t* log = this_ctx_log();
    size_t idx;
    ctx_log_record_t* rec;
    ctx_log_record_t* write_pos;
    unsigned int i;
    char symbol_buffer[KERNEL_SYMBOL_SIZE];
    
    if (nr_max_records < 1) {
        retval = -EINVAL;
        goto out_unlock;
    }
    if (nr_max_records > NR_CTX_LOG_TABLE_RECORDS) {
        nr_max_records = NR_CTX_LOG_TABLE_RECORDS;
    }
    
    unsigned int size = nr_max_records * sizeof(ctx_log_record_t);
    if (user_table == NULL || !access_ok(VERIFY_WRITE, (void *)user_table, size)) {
        retval = -EFAULT;
        goto out_unlock;
    }
    
    write_pos = user_table + (min((unsigned int)nr_max_records, CTX_LOG_NR_RECORDS(log))-1);
    cxt_log_for_each(log, rec, idx) {
        if (copy_to_user(write_pos, rec, sizeof(ctx_log_record_t))) {
            retval = -EFAULT;
            goto out_unlock;
        }
        for (i = 0; i < rec->nr_calls_trace; ++i) {
            lookup_symbol(rec->calls_trace[i].addr, symbol_buffer, KERNEL_SYMBOL_SIZE);
            if (copy_to_user(write_pos->calls_trace[i].symbol, symbol_buffer, FTRACE_SYMBOL_STR_MAX_SIZE)) {
                retval = -EFAULT;
                goto out_unlock;
            }
            write_pos->calls_trace[i].symbol[FTRACE_SYMBOL_STR_MAX_SIZE-1] = 0; // just for save - because we cut the string length.
        }
        
        write_pos--;
        
        if (++retval >= nr_max_records) {
            goto out_unlock;
        }
    }
    
out_unlock:
    spin_unlock(&rq->lock);
	return retval;
}

asmlinkage int sys_flush_ctx_log() {
    int retval = 0;
    runqueue_t *rq = this_rq_lock();  // used to lock the ctx_log
    ctx_log_t* log = this_ctx_log();
    
    if (log->biggest_written_idx == 0)
        goto out_unlock;
    
    // Assign last ctx first.
    log->table[0] = log->table[log->curr_idx.table_idx];
    log->table[0].ctx_id.id = 0;
    log->table[0].ctx_id.table_idx = 0;
    log->table[0].ctx_id.table_cycle = 0;
    
    // Set last written index to be zero.
    // Those are unsigned types so it cannot has an invalid (-1) value.
    //  always one record at least has to be written.
    log->curr_idx.id = 0;
    log->curr_idx.table_idx = 0;
    log->curr_idx.table_cycle = 0;
    log->biggest_written_idx = 0;
    
out_unlock:
    spin_unlock(&rq->lock);
	return retval;
}

DECLARE_COMPLETION(debug_work);
void complete(struct completion *x);
void wait_for_completion(struct completion *x);
asmlinkage int sys_wait_for_completion_of_debug_work() {
    wait_for_completion(&debug_work);
    return 0;
}
asmlinkage int sys_complete_debug_work() {
    complete(&debug_work);
    return 0;
}


/******** CTX LOG ********/
#endif // CTX_LOG_H
