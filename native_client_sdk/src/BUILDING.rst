Building the Full SDK
=====================

*Linux users*: Before you run build_sdk.py (below), you must run::

  GYP_DEFINES=target_arch=arm gclient runhooks

This will install some ARM-specific tools that are necessary to build the SDK.

*Everyone else*:

To build the NaCl SDK, run::

  build_tools/build_sdk.py

This will generate a new SDK in your out directory::

  $CHROME_ROOT/out/pepper_XX

Where "XX" in pepper_XX is the current Chrome release (e.g. pepper_38).

The libraries will be built, but no examples will be built by default. This is
consistent with the SDK that is shipped to users.


Testing the SDK
===============

To build all examples, you can run the test_sdk.py script::

  build_tools/test_sdk.py

This will build all examples and tests, then run tests. It will take a long
time. You can run a subset of these "phases" by passing the desired phases as
arguments to test_sdk::

  build_tools/test_sdk.py build_examples copy_tests build_tests

These are the valid phases:

* `build_examples`: Build all examples, for all configurations and toolchains.
  (everything in the examples directory)
* `copy_tests`: Copy all tests to the SDK (everything in the tests directory)
* `build_tests`: Build all tests, for all configurations and toolchains.
* `sel_ldr_tests`: Run the sel_ldr tests; these run from the command line, and
  are much faster that browser tests. They can't test PPAPI, however.
* `browser_tests`: Run the browser tests. This launches a locally built copy of
  Chrome and runs all examples and tests for all configurations. It is very
  slow.


Testing a Single Example/Test
=============================

To test a specific example, you can run the test_projects.py script::

  # Test the core example. This will test all toolchains/configs.
  build_tools/test_projects.py core

  # Test the graphics_2d example, glibc/Debug only.
  build_tools/test_projects.py graphics_2d -t glibc -c Debug

This assumes that the example is already built. If not, you can use the `-b`
flag to build it first::

  build_tools/test_projects.py nacl_io_test -t glibc -c Debug -b


Rebuilding the Projects
=======================

If you have made changes to examples, libraries or tests directory, you can
copy these new sources to the built SDK by running build_projects.py::

  build_tools/build_projects.py

You can then rebuild the example by running Make::

  cd $CHROME_ROOT/out/pepper_XX
  cd examples/api/graphics_2d  # e.g. to rebuild the Graphics2D example.
  make -j8

You can build a specific toolchain/configuration combination::

  make TOOLCHAIN=glibc CONFIG=Debug -j8

The valid toolchains are: `glibc`, `clang-newlib` and `pnacl`.
The valid configurations are: `Debug` and `Release`.

To run the example::

  # Run the default configuration
  make run

  # Run the glibc/Debug configuration
  make TOOLCHAIN=glibc CONFIG=Debug -j8

This will try to find Chrome and launch it. You can specify this manually via
the CHROME_PATH environment variable::

  CHROME_PATH=/absolute/path/to/google-chrome make run


Building Standalone Examples/Tests
-------------------------------

Building the standalone tests is often more convenient, because they are faster
to run, and don't require a copy of Chrome. We often use the standalone tests
first when developing for nacl_io, for example. However, note that most tests
cannot be built this way.

To build the standalone configuration::

  cd tests/nacl_io_test
  make STANDALONE=1 TOOLCHAIN=glibc -j8

To run the standalone tests, you must specify an architecture explicitly::

  make STANDALONE=1 TOOLCHAIN=glibc NACL_ARCH=x86_64 -j8 run
