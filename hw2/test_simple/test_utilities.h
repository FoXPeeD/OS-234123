#ifndef TEST_UTILITIES_H_
#define TEST_UTILITIES_H_

#include <stdbool.h>
#include <stdio.h>

#ifdef VERBOSE
static const bool verbose_debugging = 1;
#define SET_VERBOSE(val) do { } while(0)
#else
static bool verbose_debugging = 0;
#define SET_VERBOSE(val) do { verbose_debugging = !!(val); } while(0)
#endif

#define SET_VERBOSE_BY_ARGS(argc, argv, param_num) do {\
    int idx = (param_num);\
    if (argc >= (idx+1) && argv[idx] != NULL && atoi(argv[idx])) SET_VERBOSE(true);\
} while(0)
    
#define IS_VERBOSE() (!!verbose_debugging)

/**
 * These macros are here to help you create tests more easily and keep them
 * clear
 *
 * The basic idea with unit-testing is create a test function for every real
 * function and inside the test function declare some variables and execute the
 * function under test.
 *
 * Use the ASSERT_TEST to verify correctness of values.
 */


#define ASSERT_TEST(b,c,op) do { \
        int v1 = (b);\
        int v2 = (c);\
        if (!((v1) op (v2))) { \
                printf("\nAssertion failed at %s:%d. ![%s %s %s] ==> ![%d %s %d] [errno: %d]\n",__FILE__,__LINE__,#b,#op,#c,v1,#op,v2,errno); \
                return false; \
        } \
} while (0)

#define ASSERT_EQUALS(b, c) ASSERT_TEST(b, c, ==)
#define ASSERT_EQ(b, c) ASSERT_EQUALS(b, c)
#define ASSERT_ZERO(b) ASSERT_EQUALS(b, 0)
#define ASSERT_NEQ(b, c) ASSERT_TEST(b, c, !=)
#define ASSERT_NQ(b, c) ASSERT_NEQ(b, c)
#define ASSERT_GT(b, c) ASSERT_TEST(b, c, >)
#define ASSERT_GE(b, c) ASSERT_TEST(b, c, >=)
#define ASSERT_LT(b, c) ASSERT_TEST(b, c, <)
#define ASSERT_LE(b, c) ASSERT_TEST(b, c, <=)
#define ASSERT_POSITIVE(b) ASSERT_GT(b, 0)

/**
 * Macro used for running a test from the main function
 */
#define RUN_TEST(test) do { \
        printf("Running "#test"... "); \
        fflush(stdout); /* so "Running test ..." message won't be printed twice after fork. */ \
        if (test()) { \
            printf("[OK]\n");\
        } else { \
            printf("[%s FAILED]\n", #test);\
        }\
} while(0)


#endif /* TEST_UTILITIES_H_ */
