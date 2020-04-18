This file contains high-level info about how ChromeDriver works and how to
contribute.

ChromeDriver is an implementation of the WebDriver standard,
which allows users to automate testing of their website across browsers.

See the user site at https://sites.google.com/a/chromium.org/chromedriver/

=====Getting started=====
Build ChromeDriver by building the 'chromedriver' target. This will
create an executable binary in the build folder named
'chromedriver[.exe]'.

Once built, ChromeDriver can be used interactively with python.

$ export PYTHONPATH=<THIS_DIR>/server:<THIS_DIR>/client
$ python
>>> import server
>>> import chromedriver
>>> cd_server = server.Server('/path/to/chromedriver/executable')
>>> driver = chromedriver.ChromeDriver(cd_server.GetUrl())
>>> driver.Load('http://www.google.com')
>>> driver.Quit()
>>> cd_server.Kill()

ChromeDriver will use the system installed Chrome by default.

To use ChromeDriver2 with Chrome on Android pass the Android package name in the
chromeOptions.androidPackage capability when creating the driver. The path to
adb_commands.py and the adb tool from the Android SDK must be set in PATH. For
more detailed instructions see the user site.

=====Architecture=====
ChromeDriver is shipped separately from Chrome. It controls Chrome out of
process through DevTools. ChromeDriver is a standalone server which
communicates with the WebDriver client via the WebDriver wire protocol, which
is essentially synchronous JSON commands over HTTP. WebDriver clients are
available in many languages, and many are available from the open source
selenium/webdriver project: http://code.google.com/p/selenium. ChromeDriver
uses the webserver from net/server.

ChromeDriver has a main thread, called the command thread, an IO thread,
and a thread per session. The webserver receives a request on the IO thread,
which is sent to a handler on the command thread. The handler executes the
appropriate command function, which completes asynchronously. The create
session command may create a new thread for subsequent session-related commands,
which will execute on the dedicated session thread synchronously. When a
command is finished, it will invoke a callback, which will eventually make its
way back to the IO thread as a HTTP response for the server to send.

=====Code structure (relative to this file)=====
1) .
Implements chromedriver commands.

2) chrome/
A basic interface for controlling Chrome. Should not depend on or reference
WebDriver-related code or concepts.

3) js/
Javascript helper scripts.

4) net/
Code to deal with network communication, such as connection to DevTools.

5) client/
Code for a python client.

6) server/
Code for the chromedriver server.
A python wrapper to the chromedriver server.

7) extension/
An extension used for automating the desktop browser.

8) test/
Integration tests.

9) third_party/
Third party libraries used by chromedriver.

=====Testing=====
See the ChromeDriver waterfall at:
    http://build.chromium.org/p/chromium.chromedriver/waterfall
There are 4 test suites for verifying ChromeDriver's correctness:

1) chromedriver_unittests (chrome/chrome_tests.gypi)
This is the unittest target, which runs on the main waterfall on win/mac/linux
and can close the tree. It is also run on the commit queue and try bots by
default. Tests should take a few milliseconds and be very stable.

2) chromedriver_tests (chrome/chrome_tests.gypi)
This is a collection of C++ medium sized tests which can be run optionally
on the trybots.

3) python integration tests
Run test/run_py_tests.py --help for more info. These are only run on the
ChromeDriver waterfall.

4) WebDriver Java acceptance tests
These are integration tests from the WebDriver open source project which can
be run via test/run_java_tests.py. They are only run on the ChromeDriver
bots. Run with --help for more info.

=====Contributing=====
Find an open issue and submit a patch for review by an individual listed in
the OWNERS file in this directory. Issues are tracked in chromedriver's issue
tracker:
    https://code.google.com/p/chromedriver/issues/list
