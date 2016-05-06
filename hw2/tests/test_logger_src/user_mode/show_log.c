#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ctx_log_wrappers.h"

#define min(a,b) ((a)<(b) ? (a) : (b))

#define POLICY_STR(policy) \
    ((policy) == SCHED_OTHER ? "OTHER" : ((policy) == SCHED_FIFO ? "FIFO" : ((policy) == SCHED_RR ? "RR" : "SHORT")))
#define STATE_STR(state) \
    ((state) == TASK_RUNNING ? "RUNNING" : ((state) == TASK_INTERRUPTIBLE ? "INTERRPTBLE" : ((state) == TASK_UNINTERRUPTIBLE ? "UNINTERRPT" : ((state) == TASK_ZOMBIE ? "ZOMBIE" : "STOPPED"))))
#define LOG_MAX_SIZE 5000
ctx_log_record_t log[LOG_MAX_SIZE];

#define TASK_INFO_PRINT_FORMAT \
        "|%-6d|%-6d|%-11s|%-4d|%-4d|ts: %-4u|%-5s|%-8s|%-7s|%-4s|%-5s|%-12s|%-1d/%-1d|%-4d|%-4d|"
#define TASK_INFO_HEADER_PRINT_FORMAT \
        "|%-6s|%-6s|%-11s|%-4s|%-4s|%-8s|%-5s|%-8s|%-7s|%-4s|%-5s|%-12s|%-3s|%-4s|%-4s|"


void print_prio_queue(ctx_log_prio_queue_t* queue) {
    int i;
    
    if (queue->nr_procs == 0) {
        printf("empty\n");
        return;
    }
    
    printf("<%d> [ ", queue->prio);
    for (i = 0; i < CTX_LOG_PRIO_QUEUE_SIZE(queue); ++i) {
        printf("%d ", queue->procs[i]);
    }
    if (queue->nr_procs > NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS) {
        printf(" .. ");
        if (queue->nr_procs > NR_CTX_LOG_PRIO_QUEUE_MAX_PROCS+1) printf("%d ", queue->before_last);
        printf("%d ", queue->last);
    }
    printf("] (%d procs)\n", queue->nr_procs);
}

void print_prio_arr(ctx_log_record_t* rec, ctx_prio_array_t nr_arr) {
    if (rec->nr_active[nr_arr] < 1) return;
    printf("%s arr: %d procs. Top prio queue: ", CTX_PRIO_ARRAY_STR(nr_arr), rec->nr_active[nr_arr]);
    print_prio_queue(rec->best_prio_queues+nr_arr);
}

void print_task_info(ctx_log_rec_task_info_t* info) {
    //printf("pid: %d %s prio: %d static_prio: %d remaining_ts: %u %s %s %s %s %s %s\n",
    printf(TASK_INFO_PRINT_FORMAT "\n",
            info->pid, 
            info->ppid, 
            STATE_STR(info->state),
            info->prio, 
            info->static_prio, 
            info->remaining_timeslice, 
            POLICY_STR(info->policy), 
            info->is_interactive ? "IntrActv" : "",
            CTX_PRIO_ARRAY_STR(info->prio_array), 
            info->is_idle ? "IDLE" : "", 
            info->is_first_timeslice ? "1stTS" : "",
            info->is_overdue_forever ? "OVERDUE4EVER" : (info->is_during_overdue_period ? "OVERDUE" : ""),
            (info->policy == SCHED_SHORT ? info->remaining_cooloff_cycles : 0),
            (info->policy == SCHED_SHORT ? info->initial_cooloff_cycles : 0),
            (info->policy == SCHED_SHORT ? info->requested_time : 0),
            (info->policy == SCHED_SHORT ? info->timeslice_for_next_cooloff : 0));
    if (info->current_queue.nr_procs > 0) {
        printf("        Queue in %s array: ", CTX_PRIO_ARRAY_STR(info->prio_array));
        print_prio_queue(&info->current_queue);
    }
}

void print_task_info_header() {
    printf(TASK_INFO_HEADER_PRINT_FORMAT "\n",
            "PID", 
            "PPID", 
            "State",
            "Prio", 
            "sPri",
            "RemainTS", 
            "Plicy", 
            "IntrActv",
            "PrioArr", 
            "IDLE", 
            "1stTS",
            "OVERDUE",
            "CC", "RqTS", "NxTS");
}

int main(int argc, char** argv) {
    int print_calls_trace = (argc >= 2 && argv[1] != NULL && !strcmp(argv[1], "trace")) || (argc >= 3 && argv[2] != NULL && !strcmp(argv[2], "trace"));
    char* srcFileName = (argc >= 3 && argv[2] != NULL && strcmp(argv[2], "trace")) ? argv[2] : ((argc >= 2 && argv[1] != NULL && strcmp(argv[1], "trace")) ? argv[1] : NULL);
    int res;
    int i, k;
    ctx_log_record_t* orig_log = log; // global param
    ctx_log_record_t* log = orig_log;
    
    
    if (srcFileName != NULL) {
        log = read_ctx_log_from_file(srcFileName, &res);
        if (!log) {
            res = -1;
        }
    } else {
        res = get_ctx_log(log, LOG_MAX_SIZE);
    }
    
    if (res < 0) {
        printf("Error %d\n", errno);
        return 1;
    } else {
        printf("\n--------------------------------------------------------------------\n");
        ctx_idx_t curr_idx;
        if (get_last_ctx_id(&curr_idx) < 0) {
            printf("current idx: RETRIVAL FAULT [ErrNo: %d]   results: %d\n", errno, res);
        } else {
            printf("current ctx: [id: %u table_idx: %u :: %u]   results: %d\n", curr_idx.id, curr_idx.table_idx, curr_idx.table_cycle, res);
        }
        for (i = 0; i < res; i++) {
            if (i%5 == 0) {
                printf("--------------------------------------------------------------------\n");
                printf("        ");
                print_task_info_header();
            }
            printf("--------------------------------------------------------------------\n");
            printf("ctx_id: [id:%u | idx:%4u::%u] \tcurrent_jiffies: %u \treason: %s \t[info:%d] \n", log[i].ctx_id.id, log[i].ctx_id.table_idx, log[i].ctx_id.table_cycle, log[i].current_jiffies, CTX_REASON_STR(log[i].reason), log[i].needresched_info);
            printf("  prev: ");
            print_task_info(&(log[i].prev_info));
            printf("  next: ");
            print_task_info(&(log[i].next_info));
            
            print_prio_arr(log+i, CTX_ACTIVE);
            print_prio_arr(log+i, CTX_EXPIRED);
            print_prio_arr(log+i, CTX_SHORTS);
            print_prio_arr(log+i, CTX_OVERDUE_SHORTS);
            
            if (print_calls_trace) {
                for (k=0; k < log[i].nr_calls_trace; k++) {
                    printf("        [<%08lx>] %s (0x%x)\n", log[i].calls_trace[k].addr,log[i].calls_trace[k].symbol,log[i].calls_trace[k].stack);
                }
            }
        }
        printf("--------------------------------------------------------------------\n");
    }
}
