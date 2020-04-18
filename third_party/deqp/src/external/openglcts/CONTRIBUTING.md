OpenGL and OpenGL ES 2.0/3.X Conformance Test Contribution Guide
=================

This document describes how to add new tests to the OpenGL and OpenGL ES
2.0/3.X conformance test suites.

Contents
------------------------
- [Tips for developing new tests](#tips-for-developing-new-tests)
   - [Test framework overview](#test-framework-overview)
   - [Data Files](#data-files)
   - [Adding tests to dEQP Framework](#adding-tests-to-deqp-framework)
   - [Adding tests to GTF](#adding-tests-to-gtf)
- [Coding conventions](#coding-conventions)
- [Submitting changes](#submitting-changes)

Tips for developing new tests
------------------------
In general all new test cases should be written in the new framework residing
in the `framework` directory. Those tests should be added to the
`external/openglcts/modules` directory in the appropriate place.

See instructions below.

### Test framework overview

Tests are organized as a conceptual tree consisting of groups and, as leaves of
the tree, atomic test cases. Each node in the hierarchy has three major
functions that are called from test executor (`tcu::TestExecutor`):
1. `init()`    - called when executor enters test node
2. `iterate()` - called for test cases until `iterate()` returns `STOP`
3. `deinit()`  - called when leaving node

Each node can access a shared test context (`tcu::TestContext`). The test
context provides for example logging and resource access functionality.
Test case results are also passed to executor using the test context
(`setTestResult()`).

The root nodes are called test packages: They provide some package-specific
behavior for the TestExecutor, and often provide package-specific context for
test cases. CTS packages (except `CTS-Configs.*`) create a rendering context
in `init()` and tear it down in `deinit()`. The rendering context is passed
down in hierarchy in a package-specific `glcts::Context` object.

Test groups do not contain any test code. They usually create child nodes in
`init()`. Default `deinit()` for group nodes will destroy any created child
nodes, thus saving memory during execution.

Some test groups use a pre-defined list of children, while some may populate
the list dynamically, parsing the test script.

### Data Files

Data files are copied from source directory to build directory as a post-build
step for `glcts` target. Compiled binaries read data files
from `<workdir>/gl_cts` directory
(for example: `<workdir>/gl_cts/data/gles3/arrays.test`).

The data file copy step means that `glcts` target must be built in order to see
changes made to the data files in the source directories. On Linux this means
invoking `make` in `<builddir>`.

The data files can be included in the built binaries. See section on build
configuration for details. Android build always builds a complete APK package
with all the required files.

### Adding tests to dEQP Framework

Tests can be added to new or existing source files in `external/openglcts/modules` directory.
To register a test case into the hierarchy, the test case must be added as a
child in a test group that is already connected to the hierarchy.

There is a mini shader test framework (`glcts::ShaderLibrary`) that can create
shader cases from `*.test` files. See file `es3cTestPackage.cpp` for details on,
how to add new `*.test` files, and the existing test files in `external/openglcts/modules/gles3`
for format reference.

### Adding tests to GTF

This module is essentially frozen and should no longer be extended.

Coding conventions
------------------------
The OpenGL CTS source is formatted using [`clang-format` v4.0](http://clang.llvm.org/docs/ClangFormat.html).
Before submitting your changes make sure that the changes are formatted properly.
A recommended way to do that is to run [`clang-format-diff.py`](https://llvm.org/svn/llvm-project/cfe/trunk/tools/clang-format/clang-format-diff.py)
script on the changes, e.g.:

	cd external/openglcts && git diff -U0 HEAD^ . | python clang-format-diff.py  -style=file -i -p3 -binary clang-format-4.0

Submitting changes
------------------------
Please refer to the [Pull Requests](https://github.com/KhronosGroup/Vulkan-CTS/wiki/Contributing#pull-requests)
section of the Open GL CTS Public Wiki.
