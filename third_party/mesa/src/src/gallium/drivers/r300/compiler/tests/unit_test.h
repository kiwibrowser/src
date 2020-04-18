
struct test_result {
	unsigned int test_count;
	unsigned int pass;
	unsigned int fail;
};

struct test {
	const char * name;
	void (*test_func)(struct test_result * result);
	struct test_result result;
};

void run_tests(struct test tests[]);

void test_begin(struct test_result * result);
void test_check(struct test_result * result, int cond);
