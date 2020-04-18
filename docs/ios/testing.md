# Automated testing for Chrome for iOS

See the [instructions] for how to check out and build Chromium for iOS.

Automated testing is a crucial part of ensuring the quality of Chromium.

## Unit testing

Unit testing is done via gtests. To run a unit test, simply run the test
target (ending in _unittest).

## Integration testing

[EarlGrey] is the integration testing framework used by Chromium for iOS.

### Running EarlGrey tests

EarlGrey tests are based on Apple's [XCUITest].

#### Running tests from Xcode

An entire suite of tests can be run from Xcode.
1. Select the *egtest target you wish to run.
2. ⌘+U to run all the tests. Note: ⌘+R, which is normally used to run an
application, will simply launch the app under test, but will not run the
XCTests.

A subset of tests can be run by selecting the test or test case from the
XCTest navigator on the left side of the screen.

#### Running from the command-line

When running from the command-line, it is required to pass in the *.xctest
target, in addition to the test application.
Example:
```
./out/Debug-iphonesimulator/iossim -d "iPad Retina" -s 8.1 \
out/Debug-iphonesimulator/ios_chrome_integration_egtests.app \
out/Debug-iphonesimulator/ios_chrome_integration_egtests_module.xctest
```


[EarlGrey]: https://github.com/google/EarlGrey
[instructions]: ./build_instructions.md
[XCUITest]: https://developer.apple.com/documentation/xctest
