#include "blocker.h"
#include <assert.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

/*
 * Place two simple executable files in the following paths,
 * Make sure they exit with status 0.
 */

#define ROOT "/root/"
#define A_NAME "a.out"
#define B_NAME "bb.out"

#define PROG_A ROOT A_NAME
#define PROG_B ROOT B_NAME

void test1() {
    /* Assuming no blocked programs before the test runs. If that's not
     * the case, you should unblock any programs before */
    assert(get_blocked_count() == 0);
    assert(block_program(0, strlen(PROG_A)) == -1);
    assert(errno == EINVAL);
    assert(is_program_blocked(PROG_A, strlen(PROG_A)) == 0);
    assert(block_program(PROG_A, strlen(PROG_A)) == 0);
    assert(is_program_blocked(PROG_A, strlen(PROG_A)) == 1);
    assert(block_program(PROG_A, strlen(PROG_A)) == 1);
    assert(get_blocked_count() == 1);
    assert(block_program(PROG_B, strlen(PROG_B)) == 0);
    assert(block_program(PROG_B, strlen(PROG_B)) == 1);
    assert(get_blocked_count() == 2);

    char *argva[] = {A_NAME, 0};
    char *argvb[] = {B_NAME, 0};

    // should all fail
    execv(PROG_A, argva);
    execv(PROG_B, argvb);
    execv(PROG_A, argva);
    execv(PROG_A, argva);
    execv(PROG_B, argvb);
    execv(PROG_B, argvb);

    char log[20][256];
    assert(get_forbidden_tries(getpid(), log, 0) == -1);
    assert(errno == EINVAL);
    assert(get_forbidden_tries(-5, log, 20) == -1);
    assert(errno == ESRCH);
    assert(get_forbidden_tries(getpid(), (void*)5, 20) == -1);
    assert(errno == EFAULT);
    int n = get_forbidden_tries(getpid(), log, 20);
    assert(n == 6);
    assert(strncmp(log[0], PROG_A, strlen(PROG_A)) == 0);
    assert(strncmp(log[1], PROG_B, strlen(PROG_B)) == 0);
    assert(strncmp(log[2], PROG_A, strlen(PROG_A)) == 0);
    assert(strncmp(log[3], PROG_A, strlen(PROG_A)) == 0);
    assert(strncmp(log[4], PROG_B, strlen(PROG_B)) == 0);
    assert(strncmp(log[5], PROG_B, strlen(PROG_B)) == 0);

    assert(unblock_program(0, 11) == -1);
    assert(errno == EINVAL);
    assert(unblock_program(PROG_A, strlen(PROG_A)) == 0);
    assert(unblock_program(PROG_A, strlen(PROG_A)) == 1);
    assert(is_program_blocked(PROG_B, strlen(PROG_B)) == 1);
    assert(get_blocked_count() == 1);
    assert(unblock_program(PROG_B, strlen(PROG_B)) == 0);
    assert(is_program_blocked(PROG_A, strlen(PROG_A)) == 0);
    assert(is_program_blocked(PROG_B, strlen(PROG_B)) == 0);
    assert(unblock_program(PROG_B, strlen(PROG_B)) == 1);
    assert(is_program_blocked(PROG_A, strlen(PROG_A)) == 0);
    assert(get_blocked_count() == 0);
}

void test2() {
    assert(get_blocked_count() == 0);
    int status;

    char *argva[] = {A_NAME, 0};
    char *argvb[] = {B_NAME, 0};

    if (!fork()) {
        assert(get_blocked_count() == 0);
        execv(PROG_A, argva);
        exit(1); // shouldn't happen
    }
    wait(&status);
    assert(WEXITSTATUS(status) == 0); // assuming a.out exits with 0

    assert(block_program(PROG_A, strlen(PROG_A)) == 0);
    assert(block_program(PROG_B, strlen(PROG_B)) == 0);
    assert(block_program(PROG_A, strlen(PROG_A)) == 1);
    assert(block_program(PROG_B, strlen(PROG_B)) == 1);
    if (!fork()) {
        assert(get_blocked_count() == 2);
        execv(PROG_B, argvb);
        execv(PROG_A, argva);
        execv(PROG_B, argvb);

        int cpid = fork();
        if (!cpid) {
            char log[20][256];
            int n = get_forbidden_tries(getppid(), log, 2);
            assert(n == 2);
            assert(strncmp(log[0], PROG_B, strlen(PROG_B)) == 0);
            assert(strncmp(log[1], PROG_A, strlen(PROG_A)) == 0);
            exit(1);
        }
        int status2;
        waitpid(cpid, &status2, 0);
        assert(WEXITSTATUS(status2) == 1);
        
        exit(1);
    }
    wait(&status);
    assert(WEXITSTATUS(status) == 1);

    assert(unblock_program(PROG_A, strlen(PROG_A)) == 0);
    assert(unblock_program(PROG_B, strlen(PROG_B)) == 0);
    assert(get_blocked_count() == 0);
}

int usage() {
    if (access(PROG_A, X_OK) == -1 || access(PROG_B, X_OK) == -1) {
        printf("You should place test executable files in\n" PROG_A "\n" PROG_B "\n");
        return 1;
    }
    return 0;
}

int main() {
    if (!usage()) {
        test1();
        test2();

        // Make sure we really arrive here, otherwise it means some execv didn't fail
        printf("seems ok\n");
    }


    return 0;
}
