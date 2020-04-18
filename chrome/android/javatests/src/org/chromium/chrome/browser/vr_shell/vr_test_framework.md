# VR Test Framework

## Introduction

This is an overview of how the VR test framework functions. It is split into two
parts, with Java code located in this directory in `VrTestFramework.java` and
JavaScript code located in `//chrome/test/data/vr/e2e_test_files/`.
It is primarily used for testing WebVR, but can also be useful for testing the
VR Browser.

A similar approach is used for testing VR on desktop, with the framework
re-implemented in C++ for use in browser tests and many of the same JavaScript
files used on both platforms. See the documentation in
`//chrome/browser/vr/test` for more information on this.

## Structure

Tests utilizing this framework are split into separate Java and JavaScript files
since WebVR interaction is done via JavaScript, but instrumentation tests are
Java-based. In general, the JavaScript code handles any interactions with the
WebVR API, and the Java code handles everything else (user gestures, controller
emulation, etc.).

In general, the flow of a test is:
* In Java, load the HTML test file, which:
  * Loads the WebVR boilerplate code and test code
  * Sets up any test steps that need to be triggered by Java as separate
    functions
  * Creates an asynchronous test (denoted t here)
* Repeat:
  * Run any necessary Java-side code, e.g. trigger a user action
  * Trigger the next JavaScript test step and wait for it to finish
* Finally, call t.done() in JavaScript and endTest in Java

### JavaScript

The JavaScript test code mainly makes use of testharness.js, which is also used
for layout tests. This allows the use of asserts in JavaScript code, and any
assert failures will propagate up to Java. There are four main test-specific
functions that you will need to use:

#### async\_test

This creates an asynchronous test from testharness.js which you will use
throughout the JavaScript code. It serves two purposes:

* By being an asynchronous test, it will prevent testharness.js from ending the
  test run prematurely once the page is loaded. It will wait until all tests
  are done before checking results.
* Enables you to use asserts in JavaScript. Asserts must be within a test to
  do anything.

#### finishJavaScriptStep

This signals that the current portion of JavaScript code is done and that the
Java side can continue execution.

#### t.step

This defines a test step in an asynchronous test t. Any asserts must be within
a test step, so assertions will look along the lines of:
`t.step( () => {
  assertTrue(someBool);
});`

#### t.done

This signals that the asynchronous test is done. Once all tests in a file are
completed (usually only one), testharness.js will check the results and
automatically call finishJavaScriptStep when done.

### Java

There are many Java-side functions that enable VR testing, but only the three
basic ones will be covered here. For specifics about less common functions, see
the JavaDoc comments in `VrTestFramework.java` and the utility classes in
`util/`.

#### loadUrlAndAwaitInitialization

This is similar to the standard loadUrl method in Chrome, but also waits until
all initialization steps are complete and the page is ready for testing. By
default, this only includes the page being loaded. Additionally, any files
that include the WebVR boilerplate script (generally any test for WebVR) will
wait until the promise returned by `navigator.getVRDisplays` has resolved or
rejected. Tests can add their own initialization steps by adding an entry to
the `initializationSteps` dictionary defined in
`//chrome/test/data/android/webvr_instrumentation/resources/webvr_e2e.js` with
the value set to `false`. Once the step is done, simply set the value to
`true`.

#### executeStepAndWait

This executes the given JavaScript and waits until finishJavaScriptStep is
called on the JavaScript side.

#### endTest

Performs any post-test checks after the JavaScript test code has finished
running. Since test failures are caught after each step, all this really does
is ensure that the the JavaScript code has actually finished running, throwing
an error if this is not the case.
Typically called at the very end of the test after everything else is
completed.