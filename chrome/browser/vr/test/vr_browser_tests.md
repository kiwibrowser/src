# VR Browser Tests

## Introduction

This documentation concerns `vr_browser_test.h`, `vr_browser_test.cc`, and files
that use them.

These files port the framework used by VR instrumentation tests (located in
`//chrome/android/javatests/src/org/chromium/chrome/browser/vr_shell/` and
documented in
`//chrome/android/javatests/src/org/chromium/chrome/browser/vr_shell/*.md`) for
use in browser tests in order to test VR features on desktop platforms.

This is pretty much a direct port, with the same JavaScript/HTML files being
used for both and the Java/C++ code being functionally equivalent to each other,
so the instrumentation test's documentation on writing tests using the framework
is applicable here, too. As such, this documentation covers any notable
differences between the two implementations.

## Restrictions

Both the instrumentation tests and browser tests have hardware/software
restrictions - in the case of browser tests, VR is only supported on Windows 8
and later (or Windows 7 with a non-standard patch applied) with a GPU that
supports DirectX 11.1.

Instrumentation tests handle restrictions with the `@Restriction` annotation,
but browser tests don't have any equivalent functionality. Instead, test names
should be wrapped in the REQUIRES_GPU macro defined in `vr_browser_test.h`,
which simply disables the test by default. We then explicitly run tests that
inherit from `VrBrowserTest` and enable the running of disabled tests on bots
that meet the requirements.

## Command Line Switches

Instrumentation tests are able to add and remove command line switches on a
per-test-case basis using `@CommandLine` annotations, but equivalent
functionality does not exist in browser tests.

Instead, if different command line flags are needed, a new class will need to
be created that extends `VrBrowserTest` and overrides the flags that are set
in its `SetUp` function.