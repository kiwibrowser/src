COVERAGE_LIMIT=50

ifeq ($(QUICK),1)
TEST_ARGS?=--quick
endif

check:
	python ../../third_party/pycoverage run ./test_all.py $(TEST_ARGS)
	python ../../third_party/pycoverage report --fail-under=$(COVERAGE_LIMIT)
	python ../../third_party/pycoverage html

.PHONY: check
