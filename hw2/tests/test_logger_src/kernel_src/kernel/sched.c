
/* ... */
/*#include <linux/kernel_stat.h>*/

// uncheck to see kasserts that failed (in dmesg)
//#define HW_KDEBUG // TODO: REMOVE on submit !!!!!
#include "kassert.h"

/* CHANGE those to match your impl. */
#define TASK_SHORT(p) ((p)->policy == SCHED_SHORT)
#define TASK_OVERDUE(p) ((p)->is_overdue)
#define TASK_SHORT_OVERDUE(p) (TASK_SHORT(p) && TASK_OVERDUE(p))
#define TASK_OVERDUE_FOREVER(p) (TASK_SHORT_OVERDUE(p) && (p)->remaining_cooloff_cycles == -1)
#define TASK_SHORT_NON_OVERDUE(p) (TASK_SHORT(p) && !TASK_OVERDUE(p))
#define TASK_IDLE(p) (task_rq(p)->idle == (p))
#define OVERDUE_QUEUE_PRIO (MAX_RT_PRIO)
#define TASK_PRIO_IN_PRIO_ARR(p) (TASK_SHORT_OVERDUE(p) ? OVERDUE_QUEUE_PRIO : (p)->prio)
#define TASK_QUEUE(p, arr) ((arr)->queue + TASK_PRIO_IN_PRIO_ARR(p))

static inline size_t list_length(list_t* lst) {
    size_t size = 0;
	list_t *curr;
	list_for_each(curr, lst){
		size++;
	}
	return size;
}



/* ... */



/*static inline void rq_unlock(runqueue_t *rq) ... */

/* hw2 ctx_log */
/* Note: `ctx_reason_t` defined in sched.h */
#define ENABLE_CTX_LOG  // hw2 TODO: remove on submit !!!

#ifdef ENABLE_CTX_LOG
#include "ctx_log.h"
#else
static void ctx_logs_init() {}
void set_last_needresched_reason(task_t*, ctx_reason_t) {}
void set_last_needresched_reason_if_default(task_t*, ctx_reason_t) {}
inline static void log_ctx(task_t*, task_t*) {}
/* TODO: check if we are allowed to keep the syscalls in entry.S on submit */
asmlinkage int sys_get_last_ctx_id(ctx_idx_t*) {
    return -1;
}
asmlinkage int sys_get_ctx_log(ctx_log_record_t*, int) {
    return -1;
}
asmlinkage int sys_flush_ctx_log() {
    return -1;
}
asmlinkage int sys_wait_for_completion_of_debug_work() {
    return -1;
}
asmlinkage int sys_complete_debug_work() {
    return -1;
}
#endif


/* ... */


static int try_to_wake_up(task_t * p, int sync)
{
	/* ... */
        /*if (p->prio < rq->curr->prio) {*/
			set_last_needresched_reason(rq->curr, CTX_TRY_TO_WAKE_UP);  /* hw2 ctx_log */
            /*resched_task(rq->curr);
        }*/
	/* ... */
}

/* ... */


void wake_up_forked_process(task_t * p)
{
	/* ... */

	/*p->cpu = smp_processor_id();
	activate_task(p, rq);*/

    set_last_needresched_reason_with_info(current, CTX_DO_FORK, p->pid);  /* hw2 ctx_log */
    
    /* ... */
}

/* ... */


void scheduler_tick(int user_tick, int system)
{
	/* ... */

	/* Task might have expired already, but not scheduled off yet */
	/*if (p->array != rq->active) {*/
        set_last_needresched_reason(p, CTX_SCHEDULER_TICK);  /* hw2 ctx_log [interrupts diabled] */
		/*set_tsk_need_resched(p);
		return;
	}*/

	/* ... */

	/*if (unlikely(rt_task(p))) {*/
		/*if ((p->policy == SCHED_RR) && !--p->time_slice) {*/
			/*p->time_slice = TASK_TIMESLICE(p);
			p->first_time_slice = 0;*/
            set_last_needresched_reason(p, CTX_SCHEDULER_TICK);  /* hw2 ctx_log */
			/*set_tsk_need_resched(p);*/
			/* ... */
		/*}*/
		/*goto out;*/
	/*}*/
    
    /* ... */

	/*if (!--p->time_slice) {*/
        set_last_needresched_reason(p, CTX_SCHEDULER_TICK);  /* hw2 ctx_log */
        /* ... */
    /*}*/
}

/* ... */

asmlinkage void schedule(void)
{
	/* ... */

/*switch_tasks:
	prefetch(next);
	clear_tsk_need_resched(prev);*/

    /* hw2 ctx log */
    log_ctx(prev, next);
    
	/* if (likely(prev != next)) ... */

	/* ... */
}

/* ... */

/*#define	SLEEP_ON_HEAD					\
	/*spin_lock_irqsave(&q->lock,flags);		\*/
    set_last_needresched_reason(current, CTX_SLEEP_ON);  /* hw2 ctx_log */ \
	/*__add_wait_queue(q, &wait);			\
	spin_unlock(&q->lock);*/

/* ... */

void set_user_nice(task_t *p, long nice)
{
	/* ... */
	/*if (array) {*/
		/* ... */
            set_last_needresched_reason(rq->curr, CTX_SET_USER_NICE);  /* hw2 ctx_log */
        /* ... */
	/*}*/
	/* ... */
}


/* ... */

static int setscheduler(pid_t pid, int policy, struct sched_param *param)
{
	/* ... */
	/*if (array) {*/
		/* ... */
            set_last_needresched_reason(rq->curr, CTX_SETSCHEDULER);  /* hw2 ctx_log */
            /*resched_task(rq->curr);*/   // CODE you may add here.
        /* ... */
    /*}*/
    /* ... */
}

/* ... */

asmlinkage long sys_sched_yield(void)
{
	/* ... */
/*out_unlock:*/
    set_last_needresched_reason(current, CTX_YIELD);  /* hw2 ctx_log */
	/*spin_unlock(&rq->lock);
	schedule();*/
	/* ... */
}

/* ... */

void __init sched_init(void)
{
	/* ... */
	/*for (i = 0; i < NR_CPUS; i++) {*/
		/* ... */
        ctx_logs_init(); /* hw2 ctx_log */
	/*}*/
}
