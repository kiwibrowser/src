#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "unit_test.h"

void run_tests(struct test tests[])
{
	int i;
	for (i = 0; tests[i].name; i++) {
		printf("Test %s\n", tests[i].name);
		memset(&tests[i].result, 0, sizeof(tests[i].result));
		tests[i].test_func(&tests[i].result);
		printf("Test %s (%d/%d) pass\n", tests[i].name,
			tests[i].result.pass, tests[i].result.test_count);
	}
}

void test_begin(struct test_result * result)
{
	result->test_count++;
}

void test_check(struct test_result * result, int cond)
{
	printf("Subtest %u -> ", result->test_count);
	if (cond) {
		result->pass++;
		printf("Pass");
	} else {
		result->fail++;
		printf("Fail");
	}
	printf("\n");
}
