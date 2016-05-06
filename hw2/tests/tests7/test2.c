#include  <sys/types.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include  <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "hw2_syscalls.h"
#include "test_utilities.h"

#define SERVER_STR(is_server) ((is_server) ? "server" : "client")
#define SCHED_OTHER		0
#define SCHED_FIFO		1
#define SCHED_RR		2
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
        ASSERT_POSITIVE(mypid);
        
        nice(1); // be nicer than child
        ASSERT_ZERO(sched_setscheduler(mypid, SCHED_SHORT, &params));
        ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_SHORT, &params), -1);
        ASSERT_EQUALS(errno, EPERM);
        ASSERT_EQUALS(is_SHORT(mypid), 1);
        
        smem->arr[smem->curr] = FATHER+0; // init value
        
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



int testErrors() {
    sched_param_t params = {1, 3000, 5};
    sched_param_t params2 = {-1, 432, 4};
    sched_param_t params3 = {101, 432, 4};
    sched_param_t params4 = {1, 0, 4};
    sched_param_t params5 = {1, 3001, 4};
    sched_param_t params6 = {1, 432, -1};
    sched_param_t params7 = {1, 432, 6};
    sched_param_t params8;
    ASSERT_EQUALS(sched_setscheduler(-3, SCHED_SHORT, &params), -1);
    ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(sched_setscheduler(-3, SCHED_SHORT, NULL), -1);
    ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(sched_setscheduler(798321, SCHED_SHORT, &params), -1);
    ASSERT_EQUALS(errno, ESRCH);
/*    ASSERT_EQUALS(sched_setscheduler(0, SCHED_SHORT, &params), -1); //can we change idle??????
    ASSERT_EQUALS(errno, EPERM);
    ASSERT_EQUALS(sched_setscheduler(1, SCHED_SHORT, &params), -1); //can we change init??????
    ASSERT_EQUALS(errno, EPERM);
*/

   	int mypid = getpid();
  //  ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_SHORT, &params2), -1);
  //  ASSERT_EQUALS(errno, EINVAL);
  //  ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_SHORT, &params3), -1);
  // ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_SHORT, &params4), -1);
    ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_SHORT, &params5), -1);
    ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_SHORT, &params6), -1);
    ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_SHORT, &params7), -1);
    ASSERT_EQUALS(errno, EINVAL);

    ASSERT_EQUALS(is_SHORT(-2),-1);
   // ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(is_SHORT(mypid),-1);
    ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(is_SHORT(34324342),-1);
   // ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(remaining_time(-2),-1);
   // ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(remaining_time(mypid),-1);
    ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(remaining_time(34324342),-1);
   // ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(remaining_cooloffs(-2),-1);
  //  ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(remaining_cooloffs(mypid),-1);
    ASSERT_EQUALS(errno, EINVAL);
   ASSERT_EQUALS(remaining_cooloffs(34324342),-1);
   // ASSERT_EQUALS(errno, EINVAL);

    ASSERT_ZERO(sched_setscheduler(mypid, SCHED_SHORT, &params));
    ASSERT_EQUALS(is_SHORT(mypid), 1);
    ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_SHORT, &params), -1);
    ASSERT_EQUALS(errno, EPERM);
    ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_OTHER, &params), -1);
    ASSERT_EQUALS(errno, EPERM);
    ASSERT_EQUALS(sched_setscheduler(mypid, SCHED_RR, &params), -1);
    ASSERT_EQUALS(errno, EPERM);

   // ASSERT_EQUALS(sched_setparam(getpid(), &params2),-1);
	//ASSERT_EQUALS(errno, EINVAL);
   // ASSERT_EQUALS(sched_setparam(getpid(), &params3),-1);
	//ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(sched_setparam(getpid(), &params4),-1);
	ASSERT_EQUALS(errno, EINVAL);
    ASSERT_EQUALS(sched_setparam(getpid(), &params5),-1);
	ASSERT_EQUALS(errno, EINVAL);


	ASSERT_EQUALS(sched_getparam(getpid(), &params8),0);
	ASSERT_EQUALS(params8.importance,params.importance);
	ASSERT_EQUALS(params8.requested_time,params.requested_time);
	//ASSERT_EQUALS(params8.sched_priority,0);
	//ASSERT_EQUALS(sched_setparam(getpid(), &params6),0);
	ASSERT_EQUALS(sched_getparam(getpid(), &params8),0);
	ASSERT_EQUALS(params8.importance,params.importance);
	//ASSERT_EQUALS(params8.requested_time,params6.requested_time);
	//ASSERT_EQUALS(params8.sched_priority,0);

   	return 1;
}

int testManyForks() {
    int start = 1;
    int cooloffs = 5;
    int i=1, j=0;
    sched_param_t params = {1, 100, 1};
    sched_param_t params2;
    int mypid = getpid();
    ASSERT_POSITIVE(mypid);
    ASSERT_ZERO(sched_setscheduler(mypid, SCHED_SHORT, &params));
    for ( ; (i<= 1000); ++i) {
    start = fork();
      //mypid = getpid();
/*      for (j=0 ; ( (!start && i%2==0) || i==18) && j<753121 ; j++);
      if (i<20) {
          if (start) {
            printf("%d father is short = %d\n", i, is_SHORT(mypid));
          }
          else {
            printf("%d son is short = %d\n", i, is_SHORT(mypid));
          }
      }
*/
      params.requested_time = i;

      ASSERT_EQUALS(sched_setparam(getpid(), &params),0);
      sched_getparam(getpid(), &params2);

      ASSERT_EQUALS(params2.requested_time, i);
      if (i%2 == 0 && !start) {
        nice(1);
          mypid = getpid();
        if (is_SHORT(mypid)) {
          cooloffs = remaining_cooloffs(mypid);
        }
            exit(0);
      }
      if (i%5 == 0 && !start) {
        nice(-2);
            mypid = getpid();
            while(remaining_cooloffs(mypid));
            ASSERT_EQUALS(remaining_cooloffs(mypid),0);
            exit(0);
      }
      if (start) {
        int retval;
  //      wait(&retval);
      }
      else {
        exit(0);
      }
    }
    return 1;
}


int main(int argc, char** argv) {
    //SET_VERBOSE_BY_ARGS(argc, argv, 1);
    int res = fork();
    if (!res) {
    	int res2 = fork();
    	if (!res2) {
    		RUN_TEST(testErrors);
    	}
    	else {
    		int retval2;
        	wait(&retval2);
        	RUN_TEST(test1);
    	}
    }
    else {
    	int retval;
    	wait(&retval);
    	RUN_TEST(testManyForks);
    }
    return 0;
      /*sched_param_t params = {1, 2, 1};
      for(int i = 0; i < 8; ++i) {
        fork();
        ASSERT_ZERO(sched_setscheduler(getpid(), SCHED_SHORT, &params));
        params.requested_time = 5;
        ASSERT_EQUALS(sched_setparam(getpid(), &params),0);
        int k = 0;
        for(int i = 0; i < 300; ++i) {
         if(!is_SHORT(getpid())) {
              printf("overdue\n");
          }
          else if(is_SHORT(getpid())) {
              printf("short\n");
          }
          ++k;
        }
      }*/


    //exit(0);
        RUN_TEST(testManyForks);
        return 0;
}
