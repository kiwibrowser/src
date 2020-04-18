/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * A Simple Test Code Injection Framework.
 *
 * While small unit-tests can exercise small components in isolation,
 * some tests require a more systemic test where an independent "test
 * jig" separate from the main code can be hard to maintain.  For this
 * kind of testing, especially those needing to add "scary" test code
 * that should never be linked into production binaries, we need to
 * have a way to inject this code into the "production" code.  At the
 * same time, the addition of the injected test code should not
 * interrupt the flow of reading/auditing the production code.  To
 * this end, we define a macro that allows us to define a function
 * call to the injected test code.  This will be a no-op in production
 * linkages -- but the test for the presence of the injection will
 * remain, so that the *same* compilation is tested because the test
 * injection is done at the linkage level by supplying different
 * linkage units.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_FAULT_INJECTION_TEST_INJECTION_H_
#define NATIVE_CLIENT_SRC_TRUSTED_FAULT_INJECTION_TEST_INJECTION_H_

#include "native_client/src/include/portability.h"

/*
 * The g_nacl_test_injection_functions object cannot be an array,
 * since the type signatures of the test functions differ -- they will
 * take whatever is appropriate in the local scope of the test
 * injection site.  NB: this also makes the generality of the test
 * injection somewhat problematic, since new tests that want to inject
 * test code at the same site may potentially need additional
 * arguments, which means that adding such a new test would require
 * modifying other tests to include new arguments to maintain type
 * signature conformance.  (If we could pass in the full stack frame /
 * make the function be dynamically scoped, then this problem would
 * disappear.)
 *
 * We could approach this by adding direct function calls and use the
 * linkage unit substitution idea, but that would require a naming
 * convention to inform auditors that a function is merely for test
 * injection and is a no-op in the production linkage, which is more
 * likely to go wrong.  Using a structure in the following manner
 * enforces collecting all test injection functions together (though
 * the actual global function name / name of file/linkage unit is
 * still a convention).
 *
 * The NaCl convention for the use of this library is that
 * possibly-scary/dangerous test injection code is in a file named
 * <unit-under-test>_test_injection.c, with the
 * g_nacl_test_injection_functions global variable in a file named
 * nacl_<UUT>_test_injection_test.c and the non-injection version of
 * functions and global variable is in a single file
 * nacl_test_injection_null.c.  Since it is a link-time error to
 * provide two definitions of g_nacl_test_injection_functions, we
 * should be safe from linking in the possibly dangerous
 * <UUT>_test_injection.o files into a production binary.  Changing
 * the injection table is done NaClTestInjectionSetInjectionTable
 * below, to avoid cross-dynamic-library global variable accesses.
 */
#define NACL_TEST_INJECTION(identifier, arguments)             \
  do {                                                         \
    if (NULL != g_nacl_test_injection_functions->identifier) { \
      (*g_nacl_test_injection_functions->identifier)arguments; \
    }                                                          \
  } while (0)

struct NaClApp;

struct NaClTestInjectionTable {
  void (*ChangeTrampolines)(struct NaClApp *);
  void (*BeforeMainThreadLaunches)(void);

  /*
   * Except for -Werror=missing-field-initializers, extending this
   * structure should not affect existing tests -- since the new
   * function pointer members will get zero filled, the new call sites
   * will just not call anything in the pre-existing tests.
   */
};

extern struct NaClTestInjectionTable const *g_nacl_test_injection_functions;

void NaClTestInjectionSetInjectionTable(
    struct NaClTestInjectionTable const *new_table);

#endif
