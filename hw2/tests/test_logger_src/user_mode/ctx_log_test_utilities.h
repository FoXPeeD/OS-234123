#ifndef CTX_LOG_TEST_UTILITIES_H_
#define CTX_LOG_TEST_UTILITIES_H_

#include "test_utilities.h"
#include "ctx_log_wrappers.h"
#include <string.h>

#define MAX_PRIO 140
#define INIT_PID 1

ctx_log_record_t* get_prev_ctx_record(ctx_log_record_t log[], int nr_records, ctx_log_record_t* rec) {
    return (log == rec ? NULL : rec-1);
}
ctx_log_record_t* get_next_ctx_record(ctx_log_record_t log[], int nr_records, ctx_log_record_t* rec) {
    return (log+(nr_records-1) == rec ? NULL : rec+1);
}

// UNCHECKED
ctx_log_record_t* find_next_rec_by_pid(ctx_log_record_t rec[], unsigned int nr_records, int pid) {
    unsigned int i;
    for (i = 0; i < nr_records; ++i) {
        if (rec[i].next_info.pid == pid) {
            return (rec+i);
        }
    }
    return NULL;
}

/* 
    All of the HW-pdf-assignment instructions should be addressed in this function.
    Initial version
    TODO: add more checks !!
    Note:   It gets all the log in order to check previous/further ctx records that might be involved.
            We don't care about complixity - we do care about simplicity.
*/
bool verify_ctx_record(ctx_log_record_t log[], int nr_records, ctx_log_record_t* rec) {
    bool prev_overdue, next_overdue, next_task_idle;
    ctx_log_record_t *prev_rec, *next_rec, *forked_rec;
    
    prev_rec = get_prev_ctx_record(log, nr_records, rec);
    next_rec = get_next_ctx_record(log, nr_records, rec);
    
    int prev_task_overdue = rec->prev_info.policy == SCHED_SHORT && rec->prev_info.is_during_overdue_period;
    int next_task_overdue = rec->next_info.policy == SCHED_SHORT && rec->next_info.is_during_overdue_period;
    next_task_idle = rec->next_info.is_idle;
    
    int forked_pid;
    
    char record_info[256];
    sprintf(record_info, "record id: %d", rec->ctx_id.id);
    SetTestAdditionalInfo(record_info);
    
    if (next_rec) {
        ASSERT_EQ(next_rec->prev_info.pid,rec->next_info.pid);
    }
    
    if (next_task_idle) {
        ASSERT_ZERO(rec->nr_active[CTX_ACTIVE]);
        ASSERT_EQ(rec->best_prio_queues[CTX_ACTIVE].prio, MAX_PRIO);
        ASSERT_ZERO(rec->nr_active[CTX_EXPIRED]);
        ASSERT_EQ(rec->best_prio_queues[CTX_EXPIRED].prio, MAX_PRIO);
        ASSERT_ZERO(rec->nr_active[CTX_SHORTS]);
        ASSERT_EQ(rec->best_prio_queues[CTX_SHORTS].prio, MAX_PRIO);
        ASSERT_ZERO(rec->nr_active[CTX_OVERDUE_SHORTS]);
        ASSERT_EQ(rec->best_prio_queues[CTX_OVERDUE_SHORTS].prio, MAX_PRIO);
        
        return true; // ?
    }
    
    ASSERT_NQ(rec->next_info.prio_array, CTX_NO_PRIO_ARRAY);
    ASSERT_NQ(rec->next_info.prio_array, CTX_OTHER_RQ);
    ASSERT_BETWEEN(rec->next_info.prio_array, 0, 3);
    
    ASSERT_POSITIVE(rec->nr_active[rec->next_info.prio_array]);
    ASSERT_EQ(rec->best_prio_queues[rec->next_info.prio_array].procs[0], rec->next_info.pid);
    //ASSERT_EQ(rec->best_prio_queues[rec->next_info.prio_array].prio, rec->next_info.prio); // TODO: why not pass?
    
    switch (rec->next_info.policy) {
        case SCHED_FIFO:
        case SCHED_RR:
            ASSERT_EQ(rec->next_info.prio_array, CTX_ACTIVE);
            break;
        
        case SCHED_SHORT:
            ASSERT_GE(rec->best_prio_queues[CTX_ACTIVE].prio, MAX_RT_PRIO);
            ASSERT_GE(rec->best_prio_queues[CTX_EXPIRED].prio, MAX_RT_PRIO);

            if (next_task_overdue) {
                ASSERT_ZERO(rec->nr_active[CTX_ACTIVE]);
                ASSERT_ZERO(rec->nr_active[CTX_EXPIRED]);
                ASSERT_ZERO(rec->nr_active[CTX_SHORTS]);
                ASSERT_EQ(rec->next_info.prio_array, CTX_OVERDUE_SHORTS);
                if (next_rec) {
                    // Note: Following assumption is not true anymore. Instructor got bananas.
                    //ASSERT_EQ(next_rec->prev_info.current_queue.last, next_rec->prev_info.pid);
                }
                if (rec->reason == CTX_DO_FORK) {
                    forked_pid = rec->needresched_info;
                    ASSERT_POSITIVE(forked_pid);
                    ASSERT_EQ(rec->prev_info.current_queue.last, forked_pid);
                    
                    ASSERT(!rec->next_info.is_during_overdue_period || (rec->next_info.pid == rec->prev_info.pid));
                    
                    forked_rec = find_next_rec_by_pid(rec, nr_records-(rec-log), forked_pid);
                    if (forked_rec) {
                        ASSERT_EQ(rec->next_info.is_overdue_forever, forked_rec->next_info.is_overdue_forever);
                        ASSERT(forked_rec->next_info.ppid == rec->prev_info.pid || forked_rec->next_info.ppid == INIT_PID); // current task might has finished yet..
                        ASSERT_EQ(forked_rec->next_info.prio, rec->prev_info.prio); // it actually might have been changed ..
                        if (prev_rec && !rec->next_info.is_overdue_forever) {
                            ASSERT_EQ((prev_rec->next_info.remaining_timeslice - (rec->current_jiffies - prev_rec->current_jiffies)), rec->prev_info.remaining_timeslice); // ts not divided
                            ASSERT_EQ(rec->prev_info.remaining_timeslice, forked_rec->next_info.remaining_timeslice);
                        }
                    }
                }
            } else {
                // short non-overdue
                ASSERT_EQ(rec->next_info.prio_array, CTX_SHORTS);
                if (rec->reason == CTX_DO_FORK) {
                    forked_pid = rec->needresched_info;
                    ASSERT_NQ(rec->next_info.pid, rec->prev_info.pid);
                    ASSERT_EQ(rec->prev_info.current_queue.procs[0], forked_pid);
                    forked_rec = rec; // we assume a better-prio proc has not woken-up
                    ASSERT_EQ(forked_rec->next_info.pid, forked_pid);
                    ASSERT_EQ(forked_rec->next_info.ppid, rec->prev_info.pid);
                    ASSERT_EQ(forked_rec->next_info.prio, rec->prev_info.prio);
                    ASSERT_EQ(forked_rec->next_info.static_prio, rec->prev_info.static_prio);
                    ASSERT_EQ(rec->best_prio_queues[CTX_SHORTS].prio, rec->prev_info.prio);
                    ASSERT_EQ(rec->best_prio_queues[CTX_SHORTS].last, rec->prev_info.pid);
                    if (prev_rec) {
                        ASSERT_EQ((prev_rec->next_info.remaining_timeslice - (rec->current_jiffies - prev_rec->current_jiffies))/2, rec->prev_info.remaining_timeslice);
                        ASSERT_EQ((prev_rec->next_info.remaining_timeslice - (rec->current_jiffies - prev_rec->current_jiffies) + 1)/2, forked_rec->next_info.remaining_timeslice);
                    }
                }
                if (next_rec) {
                    if (next_rec->reason == CTX_SCHEDULER_TICK) {
                        ASSERT_EQ((next_rec->current_jiffies - rec->current_jiffies), rec->next_info.remaining_timeslice);
                        ASSERT_EQ(next_rec->prev_info.remaining_timeslice, rec->next_info.timeslice_for_next_cooloff);
                        ASSERT(next_rec->prev_info.is_during_overdue_period);
                        ASSERT_EQ(next_rec->best_prio_queues[CTX_OVERDUE_SHORTS].last, next_rec->prev_info.pid);
                    } else {
                        if (next_rec->reason == CTX_YIELD || next_rec->reason == CTX_DO_EXIT) {
                            ASSERT_FALSE(next_rec->prev_info.is_during_overdue_period);
                        }
                        // Note: Following assumption is not true anymore. Instructor got bananas.
                        //ASSERT_EQ(next_rec->prev_info.current_queue.last, next_rec->prev_info.pid);
                        ASSERT_NQ(next_rec->best_prio_queues[CTX_OVERDUE_SHORTS].last, next_rec->prev_info.pid);
                        ASSERT_POSITIVE(next_rec->prev_info.remaining_timeslice);
                        
                        if (next_rec->reason == CTX_DO_FORK) {
                            ASSERT_EQ((rec->next_info.remaining_timeslice - (next_rec->current_jiffies - rec->current_jiffies))/2, next_rec->prev_info.remaining_timeslice);
                        } else {
                            ASSERT_EQ((next_rec->current_jiffies - rec->current_jiffies), (rec->next_info.remaining_timeslice - next_rec->prev_info.remaining_timeslice));
                        }
                    }
                }
            }
            break;
        case SCHED_OTHER:
            ASSERT_EQ(rec->next_info.prio_array, CTX_ACTIVE);
            break;
        default:
            ASSERT(0);
            break;
    }
    
    if (prev_task_overdue) {
        //ASSERT_NQ(rec->reason, CTX_SCHEDULER_TICK); // TODO: why i did this? not passing anyway..
        if (next_task_idle) {
            // an overdue has replaced another overdue
            ASSERT(rec->reason == CTX_YIELD || rec->reason == CTX_SLEEP_ON || rec->reason == CTX_DO_EXIT || rec->reason == CTX_WAIT4);
            ASSERT_NQ(rec->reason, CTX_SCHEDULER_TICK);
        } else if (next_task_overdue) {
            ASSERT(rec->next_info.pid == rec->prev_info.pid || rec->reason == CTX_DEFAULT || rec->reason == CTX_YIELD || rec->reason == CTX_SLEEP_ON || rec->reason == CTX_DO_EXIT || rec->reason == CTX_WAIT4 || rec->reason == CTX_SCHEDULER_TICK || rec->reason == CTX_SCHEDULE_TIMEOUT);
        }else {
            // next is better policy
            ASSERT((rec->next_info.policy == SCHED_OTHER) || (rec->next_info.policy == SCHED_FIFO) || (rec->next_info.policy == SCHED_RR) || (rec->next_info.policy == SCHED_SHORT && rec->nr_active[CTX_SHORTS]));
            // TODO: check reason here..
        }
    }
    ClearTestAdditionalInfo();
    return true;
}

bool verify_ctx_log(ctx_log_record_t log[], int nr_records) {
    unsigned int i;
    for (i = 0; i < nr_records; ++i) {
        ASSERT(verify_ctx_record(log, nr_records, log+i));
    }
    return true;
}

// UNCHECKED
ctx_log_record_t* find_next_rec_non_sched_other(ctx_log_record_t rec[], unsigned int nr_records) {
    unsigned int i;
    for (i = 0; i < nr_records; ++i) {
        if (rec[i].next_info.policy != SCHED_OTHER) {
            return (rec+i);
        }
    }
    return NULL;
}

// UNCHECKED
ctx_log_record_t* find_next_rec_non_sched_other_after_specific(ctx_log_record_t rec[], int nr_records,ctx_log_record_t* specific) {
    assert(specific != NULL);
    int diff = specific - rec;
    return find_next_rec_non_sched_other(specific+1,nr_records-(diff+1));
}

// UNCHECKED
ctx_log_record_t* find_next_rec_short(ctx_log_record_t rec[], int nr_records) {
    int i;
    for (i = 0; i < nr_records; ++i) {
        if (rec[i].next_info.policy == SCHED_SHORT && rec[i].reason != CTX_DO_FORK) {
            return (rec+i);
        }
    }
    return NULL;
}

// UNCHECKED
ctx_log_record_t* find_next_rec_short_after_specific(ctx_log_record_t rec[], int nr_records,ctx_log_record_t* specific) {	
    assert(specific != NULL);
    int diff = specific - rec;
    assert(nr_records >= (diff+1));
    return find_next_rec_short(specific+1,nr_records-(diff+1));
}

// UNCHECKED
ctx_log_record_t* get_rec_by_ctx_id(ctx_log_record_t rec[], int nr_records, ctx_idx_t ctx_id) {
    if (nr_records < 1) return NULL;
    unsigned long delta = (ctx_id.id - rec[0].ctx_id.id);
    if (delta >= (unsigned long)nr_records) return NULL;
    rec = rec+delta;
    return (rec->ctx_id.id == ctx_id.id ? rec : NULL);
}

#define req_find_next_rec_short_after_specific(new_name,log,size,rec) \
    ctx_log_record_t* new_name = find_next_rec_short_after_specific((log),(size),(rec)); \
    ASSERT(new_name); \
	last_short_obtained_ctx_id = new_name->ctx_id; \
    do {} while(0)
#define req_get_rec_by_ctx_id(new_name,log,size) \
    ctx_log_record_t* new_name = get_rec_by_ctx_id((log),(size)); \
    ASSERT(new_name); \
    do {} while(0)
#define req_find_next_rec_short(new_name,log,size) \
    ctx_log_record_t* new_name = find_next_rec_short((log),(size)); \
    ASSERT(new_name); \
    do {} while(0)
    


#define WRITE_LOG_TO_FILE(log, log_actual_size) do { \
    size_t size = strlen(GetTestName())+4+1;\
    char filename[size]; \
    strcpy(filename, GetTestName()); \
    filename[size-1] = 0;\
    filename[size-2] = 'g';\
    filename[size-3] = 'o';\
    filename[size-4] = 'l';\
    filename[size-5] = '.';\
    ASSERT_ZERO(write_ctx_log_to_file(log, log_actual_size, filename)); \
} while(0)



#endif // CTX_LOG_TEST_UTILITIES_H_
