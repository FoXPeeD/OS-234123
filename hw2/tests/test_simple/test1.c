#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "hw2_syscalls.h"
#include "test_utilities.h"

#define SERVER_STR(is_server) ((is_server) ? "server" : "client")
#define SCHED_SHORT 5
#define FATHER          10000
#define SON             20000
#define OVERDUE_PERIOD  1000
#define max(a,b) ((a)>(b) ? (a) : (b))

typedef struct sched_param {
    int sched_priority;
    int requested_time;
    int importance;
} sched_param_t;

void* make_shared(size_t size, int is_server) {
    int       shm_id;
    key_t     mem_key;
    void       *shm_ptr;

    mem_key = ftok("/tmp", 'b');
    shm_id = shmget(mem_key, size, ((is_server ? IPC_CREAT : 0) | 0666));
    if (shm_id < 0) {
         printf("*** shmget error (%s) ***\n", SERVER_STR(is_server));
         exit(1);
    }

    shm_ptr = shmat(shm_id, NULL, 0);  /* attach */
    if ((int) shm_ptr == -1) {
         printf("*** shmat error (%s) ***\n", SERVER_STR(is_server));
         exit(1);
    }
    return shm_ptr;
}

#define N 40
typedef struct data {
    size_t curr;
    int arr[N];
} data_t;

int test1() {
    int child = fork();
	
    int i;
    data_t* smem;
    pid_t mypid;
    int cooloffs = 5;
    sched_param_t params = {1, 50, cooloffs};
    int status;
    int counter = 0;
        
    if (child != 0) {
        smem = (data_t*)make_shared(sizeof(data_t), 1);
        smem->curr = 0;
        
        mypid = getpid();
		printf("mypid (son) = %d\n",mypid);
        ASSERT_POSITIVE(mypid);
        nice(1); // be nicer than child
        ASSERT_ZERO(sched_setscheduler(mypid, SCHED_SHORT, &params));
        ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_SHORT, &params), -1);
        ASSERT_EQUALS(errno, EPERM);
        ASSERT_EQUALS(is_SHORT(mypid), 1);
        
        smem->arr[smem->curr] = FATHER+0; // init value
        //return 0;
        ASSERT_ZERO(sched_setscheduler(child, SCHED_SHORT, &params)); // now we lost control until child will be overdue
        
        // child got into overdue. we gained control again. we should still be short here.
        smem->arr[++smem->curr] = FATHER+1;
        while (is_SHORT(mypid)) ;
        smem->arr[++smem->curr] = FATHER+(1*10)+OVERDUE_PERIOD; // got into first overdue period
        ASSERT_EQUALS(remaining_cooloffs(mypid), cooloffs-1);

        for (i = 1; i <= cooloffs; ++i) {
            while (!is_SHORT(mypid)) ;
            smem->arr[++smem->curr] = FATHER+(i*10); // got out of overdue period
            ASSERT_EQUALS(remaining_cooloffs(mypid), cooloffs-i);
            while (is_SHORT(mypid)) ;
            smem->arr[++smem->curr] = FATHER+((i+1)*10)+OVERDUE_PERIOD; // got into overdue period
            ASSERT_EQUALS(remaining_cooloffs(mypid), max(cooloffs-(i+1), 0));
        }
        
        // now should be overdue forever
        ASSERT_ZERO(remaining_cooloffs(mypid));
        
        waitpid(child, &status, 0);
        
        // use `gcc -DVERBOSE ...` in order to print the array state
        if (IS_VERBOSE()) {
            for (i = 0; i <= 24; i++) {
                printf("%d:\t%d\n", i, smem->arr[i]);
            }
        }
        
        // check array
        ASSERT_EQUALS(smem->arr[0], FATHER+0);
        ASSERT_EQUALS(smem->arr[1], SON+0);
        ASSERT_EQUALS(smem->arr[2], FATHER+1);
        ASSERT_EQUALS(smem->arr[3], SON+10+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[4], SON+10);
        ASSERT_EQUALS(smem->arr[5], FATHER+10+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[6], FATHER+10);
        ASSERT_EQUALS(smem->arr[7], SON+20+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[8], SON+20);
        ASSERT_EQUALS(smem->arr[9], FATHER+20+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[10], FATHER+20);
        ASSERT_EQUALS(smem->arr[11], SON+30+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[12], SON+30);
        ASSERT_EQUALS(smem->arr[13], FATHER+30+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[14], FATHER+30);
        ASSERT_EQUALS(smem->arr[15], SON+40+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[16], SON+40);
        ASSERT_EQUALS(smem->arr[17], FATHER+40+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[18], FATHER+40);
        ASSERT_EQUALS(smem->arr[19], SON+50+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[20], SON+50);
        ASSERT_EQUALS(smem->arr[21], FATHER+50+OVERDUE_PERIOD);
        ASSERT_EQUALS(smem->arr[22], FATHER+50);
        ASSERT_EQUALS(smem->arr[23], SON+60+OVERDUE_PERIOD);    // son finished
        ASSERT_EQUALS(smem->arr[24], FATHER+60+OVERDUE_PERIOD);
        
        return 1;
        
    } else {
        pid_t mypid = getpid();
		printf("mypid (parent) = %d\n",mypid);
        ASSERT_POSITIVE(mypid);
        
        while (is_SHORT(mypid) != 1) ;
        
        data_t* smem = (data_t*)make_shared(sizeof(data_t), 0);
        
        smem->arr[++smem->curr] = SON+0; // this is the first SHORT time slice
        
        while (is_SHORT(mypid)) ;
        smem->arr[++smem->curr] = SON+(1*10)+OVERDUE_PERIOD; // got into first overdue period
        ASSERT_EQUALS(remaining_cooloffs(mypid), cooloffs-1);
        
        for (i = 1; i <= cooloffs; ++i) {
            while (!is_SHORT(mypid)) ;
            smem->arr[++smem->curr] = SON+(i*10); // got out of overdue period
            ASSERT_EQUALS(remaining_cooloffs(mypid), cooloffs-i);
            while (is_SHORT(mypid)) ;
            smem->arr[++smem->curr] = SON+((i+1)*10)+OVERDUE_PERIOD; // got into overdue period
            ASSERT_EQUALS(remaining_cooloffs(mypid), max(cooloffs-(i+1), 0));
        }
        
        // now should be overdue forever
        ASSERT_ZERO(remaining_cooloffs(mypid));
        
        exit(0);
    }

    return 0;
}

int main(int argc, char** argv) {
    SET_VERBOSE_BY_ARGS(argc, argv, 1);
    
    RUN_TEST(test1);
    
    return 0;
}
