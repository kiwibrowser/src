This directory contains extensions which are unit tests for the extension API.
These tests are written using the extension API test framework, which allows
us to do end-to-end testing of extension API in a browser_test.  The general way
these tests work is to run code in an extension when they're loaded and to post
a pass or fail notification back up to the C++ unit test which then reports the
success or failure.  In the common case, the extension runs many subtests and
then reports up a single pass or fail.  This case is made easy by the test
framework.

To write a new test:

(1) Add a new browser_test which is a subclass of ExtensionApiTest.  This test
should call RunExtensionTest("extension_name") to kick off the test.  See
bookmark_extension_apitest.cc for an example.

(2) Create an extension of in this directory of the same name as the extension
that your test referred to ("extension_name" above).  This test should load
a background page which immediately starts its test.

(3) In your extension page, call chrome.test.runTests with an array of
functions which represent your subtests.  Each of these functions will most
likely call one or more async extension APIs.  Wrap the callback for each of
these API calls with chrome.test.callbackPass or chrome.test.callbackFail
depending on whether or not you're expecting the callback to generate an error
or not.  That's it.  The test framework notices when each of these callbacks
is registered and keeps a count of what's expected.  When the right number of
callbacks has fired, that test function will be marked as passed or failed and
the next one will be called.  Some other useful helper functions you'll use are
chrome.test.assertTrue(expr, message), chrome.test.assertEq(left, right) and
chrome.test.log(message).

Here's an example:

chrome.test.runTests([
  function getTree() {
    chrome.bookmarks.getTree(chrome.test.callbackPass(function(results) {
      chrome.test.assertTrue(compareTrees(results, expected),
                             "getTree() result != expected");
    }));
  },

  function get() {
    chrome.bookmarks.get("1", chrome.test.callbackPass(function(results) {
      chrome.test.assertTrue(compareNode(results[0], expected[0].children[0]));
    }));
    chrome.bookmarks.get("42", chrome.test.callbackFail("Can't find bookmark for id."));
  },

  function getArray() {
    chrome.bookmarks.get(["1", "2"], chrome.test.callbackPass(function(results) {
      chrome.test.assertTrue(compareNode(results[0], expected[0].children[0]),
                             "get() result != expected");
      chrome.test.assertTrue(compareNode(results[1], expected[0].children[1]),
                             "get() result != expected");
    }));
  }
]);

// compareNode and compareTrees are helper functions that the bookmarks test
// uses for convenience.  They're not really relevant to the framework itself.

Note that chrome.test.callbackFail takes an argument which is the error message
that it expects to get when the callback fails
(chrome.runtime.lastError.message).

Here's what the output of this test might look like:
[==========] Running 1 test from 1 test case.
[----------] Global test environment set-up.
[----------] 1 test from ExtensionApiTest
[ RUN      ] ExtensionApiTest.Bookmarks
( RUN      ) getTree
(  SUCCESS )
( RUN      ) get
(  SUCCESS )
( RUN      ) getArray
(  SUCCESS )
Got EXTENSION_TEST_PASSED notification.
[       OK ] ExtensionApiTest.DISABLED_Bookmarks (2472 ms)
[----------] 1 test from ExtensionApiTest (2475 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test case ran. (2482 ms total)
[  PASSED  ] 1 test.
1 test run
0 test failed

Note the RUN/SUCCESS messages in () - these are the subtests that are run in
the extension itself.  Anything printed with chrome.test.log() will also display
in stdout of the browser test (and hence in the buildbot output for that test)
if the VLOG level for extensions/browser/api/test/test_api.cc is at least 1,
which can be done e.g. by adding the command-line switch --vmodule=*test_api*=1,
see extensions/browser/api/test/test_api.cc.
