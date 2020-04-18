# LayoutTests for Accessibility

## General Info on LayoutTests: Building and Running the Tests

See [Layout Tests](/docs/testing/layout_tests.md) for general
info on how to build and run layout tests.

## Old vs. New

There are two styles of accessibility layout tests:

* Using a ```-expected.txt``` (now deprecated)
* Unit-style tests with assertions

Use the unit-style tests. An example is aria-modal.html.

## Methodology and Bindings

These tests check the accessibility tree directly in Blink using ```AccessibilityController```, which is just a test helper.

The code that implements the bindings is here:

* ```content/shell/test_runner/accessibility_controller.cc```
* ```content/shell/test_runner/web_ax_object_proxy.cc```

You'll probably find bindings for the features you want to test already. If not, it's not hard to add new ones.
