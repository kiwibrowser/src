# VR Instrumentation Tests

## Introduction

This directory contains all the Java-side infrastructure for running
instrumentation tests on the two virtual reality (VR) features currently
in Chrome:

* [WebVR](https://webvr.info/) - Experience VR content on the web
* VR Browser - Browse the 2D web from a VR headset

## Directories

These are the files and directories that are relevant to VR instrumentation
testing. Additional information on files in other directories can be found in
those directories' README.md files (if the files as a whole warrant special
documentation) and the JavaDoc comments in each file.

### Subdirectories

* `mock/` - Contains all the classes for mock implementations of VR classes
* `nfc_apk/` - Contains the code for the standalone APK for NFC simulation. Used
  by Telemetry tests, not instrumentation tests, but kept here since it uses
  code from util/
* `rules/` - Contains all the VR-specific JUnit4 rules
* `util/` - Contains utility classes with code that's used by multiple test
  classes

### Other Directories

* `//chrome/android/shared_preference_files/test/` - Contains the VrCore settings
  files for running VR instrumentation tests (see the "Building and Running"
  section for more information).
* `//chrome/test/data/vr/e2e_test_files/` - Contains the JavaScript
  and HTML files for VR instrumentation tests.
* `//third_party/gvr-android-sdk/test-apks/` - Contains the VrCore and Daydream
  Home APKs for testing. You must have `DOWNLOAD_VR_TEST_APKS` set as an
  environment variable when you run gclient sync in order to actually download
  these from storage.
* `//third_party/gvr-android-sdk/test-libraries/` - Contains third party VR
  testing libraries. Currently, only has the Daydream controller test library.

## Building and Running

The VR instrumentation tests can be built with the `chrome_public_test_vr_apk`
target, which will also build `chrome_public_apk` to test with.

Once both are built, the tests can be run by executing
`run_chrome_public_test_vr_apk` in your output directory's `bin/` directory.
However, unless you happen to have everything set up properly on the device, the
tests will most likely fail. To fix this, you just need to pass a few extra
arguments to `run_chrome_public_test_vr_apk`.

**NOTE** These tests can only be run on rooted devices.

### Standard Arguments

These are the arguments that you'll pretty much always want to pass in order to
ensure that your device is set up properly for testing.

#### additional-apk

`--additional-apk path/to/apk/to/install`

All this does is install the specified APK before running tests. You'll
generally want to install the current version of VrCore by passing it

`--additional-apk
third_party/gvr-android-sdk/test-apks/vr_services/vr_services_current.apk`

Just make sure that the APK is up to date and on your system by running gclient
sync with the `DOWNLOAD_VR_TEST_APKS` environment variable set. This argument can
be ommitted if you're fine with using whatever version of VrCore is already
installed on the device.

**NOTE** This will fail on most Pixel devices, as VrCore is pre-installed as a
system app. You can get around this in two ways.

* Remove VrCore as a system app by following the instructions
  [here](go/vrcore/building-and-running). This will permanently solve the issue
  unless you reflash your device.
* Use `--replace-system-package
  com.google.vr.vrcore,//third_party/gvr-android-sdk/test-apks/vr_services/vr_services_current.apk`
  instead. This will take significantly longer, as it requires rebooting, and
  must be done every time you run the tests.

#### shared-prefs-file

`--shared-prefs-file path/to/preference/json/file`

This will configure VrCore according to the provided file, e.g. changing the
paired headset. The two most common files to use are:

* `//chrome/android/shared_preference_files/test/vr_cardboard_skipdon_setupcomplete.json`
  This will pair the device with a Cardboard headset and disabled controller
  emulation.
* `//chrome/android/shared_preference_files/test/vr_ddview_skipdon_setupcomplete.json`
  This will pair the device with a Daydream View headset, set the DON flow to be
  skipped, and enable controller emulation. **NOTE** This will prevent you from
  using a real controller outside of tests. To fix this, you can either
  reinstall VrCore or apply the Cardboard settings file (there isn't currently a
  way to manually re-enable real controller use from within the VrCore developer
  settings)

### Other Useful Arguments

#### test-filter

`--test-filter TestClass#TestCase`

If you only have interest in a particular test case or class, such as when you
add a new test or are trying to reproduce a failure, you can significantly cut
down on the test runtime by limiting the tests that are run. You can either
specify a specific test case to restrict to, or use \* to restrict to all test
cases within a test class.

## Creating New Tests

### Adding A Test Case To An Existing Test Class

If you're new to adding VR instrumentation tests or instrumentation tests in
general, it's a good idea to take a look at existing tests to get a feel for
what's going on. In general, these are the things that you need to be aware of
when adding a new test:

#### @Test

Every test case must be annotated with the `@Test` annotation in order to be
identified as a test.

#### Test Length

Every test case must also have a test length annotation, typically
`@MediumTest`. Eventually, the test length annotations should imply the presence
of `@Test` as well, but for now, both must be present.

#### VR Test Framework

Most test classes will define a `VrTestFramework` member as `mVrTestFramework`,
which contains functions that are critical for WebVR testing, and still useful
for VR Browser testing. Examples include:

* Retrieving HTML test files
* Accessing the WebContents and ContentViewCore of whatever tab the test started
  on, which is where the vast majority of testing takes place
* Waiting on JavaScript test steps and running arbitrary JavaScript code

If you are writing a test that uses WebVR, you will likely also need to add an
HTML file to `//chrome/test/data/vr/e2e_test_files/html/` that
handles the JavaScript portion of the test. In general, a WebVR test will have a
number of steps as functions on the JavaScript side. The Java side of the test
will load the file and start a JavaScript step, blocking until it finishes. The
JavaScript code will execute and call `finishJavaScriptStep()` when done, which
will signal the Java code to continue. This repeats until the test is done. For
more specifics, see `vr_test_framework.md`.

#### VR Test Rule

Every test class will have a `mVrTestRule` member that inherits from
`ChromeActivityTestRule`. It does some important setup under the hood, but can
also be used to interact with Chrome during a test. If you need to interact with
Chrome and `VrTestFramework` does not have a way of doing that, then there is
likely a way to do it using `mVrTestRule`.

#### Test Parameterization

If a test class is annotated with `@RunWith(ParameterizedRunner.class)`, then it
has support for test parameterization. While parameterization has many potential
uses, the current use in VR is to allow a test case to be automatically run in
multiple different activity types. For specifics, see
`rules/README.md`.

If a class has test parameterization enabled, you have three options when adding
a new test:

* Do nothing special. This will default to only running the test in
  ChromeTabbedActivity, which is the normal Chrome browser.
* Add the `@VrActivityRestriction` annotation with
  `VrActivityRestriction.SupportedActivity.ALL` as its value, which will run the
  test in all activities that support VR
* Add the `@VrActivityRestriction` annotation, manually specifying the
  activities you want the test to run in

### Adding A New Test Class

Adding a new test class is similar to adding a test case to an existing class,
but with a few extra steps.

#### Determine Whether To Enable Test Parameterization

Adding test parameterization will allow tests in the class to be automatically
run in multiple activities that support VR. However, enabling it on a test class
adds a non-trivial amount of runtime overhead, so you should only enable it in
classes where it actually makes sense. In general, this boils down to whether
your test class will have WebVR tests in it or not - WebVR is supported in
multiple activity types, but VR Browsing is only supported in
ChromeTabbedActivity.

Whether test parameterization is enabled or not only affects the boilerplate
code in the test class - test cases themselves do not care whether the feature
is enabled or not.

##### Non-Parameterized

See `VrShellNavigationTest.java` for an example of how to set up non-parameterized
test classes. In general, you will need to:

* Set `@RunWith` to `ChromeJUnit4ClassRunner.class`
* Define `mVrTestRule` as a `ChromeTabbedActivityVrTestRule`, annotate it with
  `@Rule`, and initialize it where it's defined
* Define `mVrTestFramework` as a VrTestFramework and initialize it using
  `mVrTestRule` in a setup function annotated with `@Before`

  Initializing this in the setup function is necessary since the test framework
  needs `mVrTestRule` to be applied, which is guaranteed to have occurred by the
  time the setup function is run.

##### Parameterized

See `WebVrTransitionTest.java` for an example of how to set up parameterized
test classes. In general, you will need to:

* Set `@RunWith` to `ParameterizedRunner.class`
* Add `@UseRunnerDelegate` and set it to `ChromeJUnit4RunnerDelegate.class`
* Define `sClassParams`, annotate it with `@ClassParameter`, and set it to the
  value returned by `VrTestRuleUtils.generateDefaultVrTestRuleParameters()`
* Define `mRuleChain` as a `RuleChain` and annotate it with `@Rule`
* Define `mVrTestRule` as a `ChromeActivityTestRule`
* Define `mVrTestFramework` as a `VrTestFramework` and initialize it using
  `mVrTestRule` in a setup function annotated with `@Before`
* Define a constructor for your test class that takes a
  `Callable<ChromeActivityTestRule>`. This constructor must set `mVrTestRule` to
  the callable's `call()` return value and set `mRuleChain` to the return value
  of `VrTestruleUtils.wrapRuleInVrActivityRestrictionRule(mVrTestRule)`

#### Include New Class In Build

Add the new test class to the `java_files` list of the `chrome_test_vr_java`
target in `//chrome/android/BUILD.gn`
