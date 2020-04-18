harness_TESTS =					\
	da-dk-g16-dictionary_harness.txt	\
	da-dk-g18-dictionary_harness.txt	\
	da-dk-g26-dictionary_harness.txt	\
	da-dk-g28-dictionary_harness.txt	\
	en-GB-g2_harness.txt			\
	en-gb-g1_harness.txt			\
	en-ueb-g2-dictionary_harness.txt	\
	en-us-g2-dictionary_harness.txt		\
	ethio-g1_harness.txt			\
	hu-hu-comp8_harness.txt			\
	hu-hu-g1-hyph_harness.txt		\
	hu-hu-g1_harness.txt			\
	iu-ca-g1_harness.txt			\
	ko-g2_harness.txt			\
	letterDefTest_harness.txt		\
	nocross_harness.txt			\
	ta-ta-g1_harness.txt			\
	chr-us-g1_harness.txt

EXTRA_DIST = $(harness_TESTS)			\
	     runHarness.py

HARNESS_DIR = $(top_srcdir)/tests/harness

harness_ENVIRONMENT =												\
	HARNESS_DIR=$(HARNESS_DIR)										\
	LOUIS_TABLEPATH=$(top_srcdir)/tables,$(top_srcdir)/tests/tables,$(top_srcdir)/tests/tables/moreTables	\
	PYTHONPATH=$(HARNESS_DIR):$(top_srcdir)/python:$$PYTHONPATH						\
	LD_LIBRARY_PATH=$(top_srcdir)/liblouis/.libs:$$LD_LIBRARY_PATH						\
	PATH=$(top_srcdir)/tools:$$PATH

if HAVE_PYTHON
if HAVE_UCS4
runall runAll:
	@$(harness_ENVIRONMENT) ./runHarness.py

$(harness_TESTS):
	@$(harness_ENVIRONMENT) ./runHarness.py $@
else
runall runAll:
	@echo Please configure with --enable-ucs4
endif
else
runall runAll:
	@echo Python is required to run the harness tests.
endif

check:
	@true

.PHONY: runall runAll $(harness_TESTS)

