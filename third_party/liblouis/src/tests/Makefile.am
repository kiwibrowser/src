SUBDIRS = tables doctests harness harnessSources

AM_CPPFLAGS = -I$(top_srcdir)/liblouis

LDADD = $(top_builddir)/liblouis/liblouis.la

present_progressive_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	present_progressive.c

capitalized_word_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	capitalized_word.c

capitalized_with_sentance_SOURCES =		\
	brl_checks.c				\
	brl_checks.h				\
	capitalized_with_sentance.c

inpos_compbrl_SOURCES =		\
	brl_checks.c            \
	brl_checks.h            \
	inpos_compbrl.c	

lastworditalafter_SOURCES =	\
	brl_checks.c            \
	brl_checks.h            \
	lastworditalafter.c	

en_gb_g1_italics_SOURCES =	\
	brl_checks.c            \
	brl_checks.h            \
	en_gb_g1_italics.c	

pass2_SOURCES =			\
	brl_checks.c		\
	brl_checks.h		\
	pass2.c	

pass2_inpos_SOURCES =		\
	brl_checks.c		\
	brl_checks.h		\
	pass2_inpos.c	

inpos_SOURCES =			\
	brl_checks.c		\
	brl_checks.h		\
	inpos.c	

inpos_match_replace_SOURCES =	\
	brl_checks.c		\
	brl_checks.h		\
	inpos_match_replace.c

letterDefTest_SOURCES = 	\
	brl_checks.c		\
	brl_checks.h		\
	letterDefTest.c

uplow_with_unicode_SOURCES = 	\
	brl_checks.c		\
	brl_checks.h		\
	uplow_with_unicode.c

squash_space_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	squash_space.c

hyphenate_achena_SOURCES  = 		\
	brl_checks.c			\
	brl_checks.h			\
	hyphenate_achena.c

hyphenate_alderen_SOURCES  = 		\
	brl_checks.c			\
	brl_checks.h			\
	hyphenate_alderen.c

hyphenate_straightforward_SOURCES  =		\
	brl_checks.c				\
	brl_checks.h				\
	hyphenate_straightforward.c

hyphenate_xxx_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	hyphenate_xxx.c

backtranslate_with_letsign_SOURCES =		\
	brl_checks.c				\
	brl_checks.h				\
	backtranslate_with_letsign.c

backtranslate_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	backtranslate.c

pass1Only_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	pass1Only.c

outpos_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	outpos.c	

getTable_SOURCES = getTable.c

typeform_SOURCES =				\
	typeform.c

pass0_typebuf_SOURCES =				\
	brl_checks.c				\
	brl_checks.h				\
	pass0_typebuf.c

hash_collision_SOURCES =			\
	brl_checks.c				\
	brl_checks.h				\
	hash_collision.c

resolve_table_SOURCES =				\
	resolve_table.c

logging_SOURCES =				\
	logging.c

check_PROGRAMS =				\
	pass2					\
	pass2_inpos				\
	inpos					\
	inpos_match_replace			\
	uplow_with_unicode			\
	present_progressive			\
	capitalized_word			\
	capitalized_with_sentance		\
	inpos_compbrl				\
	lastworditalafter			\
	en_gb_g1_italics			\
	letterDefTest				\
	squash_space				\
	hyphenate_achena			\
	hyphenate_alderen			\
	hyphenate_straightforward		\
	hyphenate_xxx				\
	backtranslate_with_letsign 		\
	backtranslate 				\
	pass1Only				\
	outpos					\
	getTable				\
	typeform				\
	pass0_typebuf				\
	hash_collision                          \
	resolve_table                          \
	logging


dist_check_SCRIPTS =		\
	check_all_tables.pl	\
	check_endless_loop.pl	\
	multiple_table_path.pl

# if we have Python and liblouis is configured with ucs4 then we can
# invoke the python based tests
if HAVE_PYTHON
if HAVE_UCS4
dist_check_SCRIPTS += \
	check_doctests.py
endif
endif

# The present_progressive should not really fail but for the moment
# the issue doesn't seem to be critical enough that anybody has enough
# interest to fix it.
XFAIL_TESTS =					\
	present_progressive			\
	lastworditalafter			\
	en_gb_g1_italics			\
	squash_space

# The hash_collision currently fails. The first translation works but subsequent ones fail. 
XFAIL_TESTS += hash_collision

TESTS =				\
	$(check_PROGRAMS)	\
	$(dist_check_SCRIPTS)

TESTS_ENVIRONMENT =												\
	LOUIS_TABLEPATH=$(top_srcdir)/tables,$(top_srcdir)/tests/tables,$(top_srcdir)/tests/tables/moreTables	\
	PYTHONPATH=$(HARNESS_DIR):$(top_srcdir)/python:$$PYTHONPATH						\
	HARNESS_DIR=$(HARNESS_DIR)										\
	LD_LIBRARY_PATH=$(top_srcdir)/liblouis/.libs:$$LD_LIBRARY_PATH						\
	PATH=$(top_srcdir)/tools:$$PATH
