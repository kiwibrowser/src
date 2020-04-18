// Copyright 2018 The Native Client SDK Authors.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

var retries = 200;
var retryWait = 50;

/**
 * Determine if a NaCl module associated with an EMBED element has been
 * loaded.
 * @param {!Element} embed The EMBED element to check.
 * @return {bool} whether the module is loaded.
 */
function isLoaded(embed) {
  return embed.readyState == 4;
}

/**
 * See if a NaCl module is loaded, and if not start a timer that polls
 * the EMBED element associated with the module.  Fail after |retries|
 * attempts.
 * @param {!Element} embed The EMBED element to test.
 * @param {!function} moduleDidLoad This callback is called when the
 *      NaCl module isloaded, or after the number of retires is exceeded.
 *      The callback is passed a bool argument indicating load success.
 */
function waitForNaClModule(embed, moduleDidLoad, responseCallback) {
  if (isLoaded(embed)) {
    moduleDidLoad(true, responseCallback);
    return;
  }
  retries -= 1;
  if (retries <= 0) {
    moduleDidLoad(false, responseCallback);
    return;
  }
  // At this point, there is no discernible load error, and the module
  // has not yet loaded.  Continue to wait.
  responseCallback({type: 'log', message: 'Waiting...'});
  var callback = function() {
    waitForNaClModule(embed, moduleDidLoad, responseCallback);
  };
  setTimeout(callback, retryWait);
}

/**
 * Once the NaCl module is loaded, run some tests on it by attempting to
 * access a property and by calling a method.  Once these results are
 * obtained, send a message back to the embedding page with the results
 * via an RPC.  This function is provided as the callback to
 * waitForNaClModule().
 * @param {bool} loadSuccess Indicates whether the NaCl module was
 *     loaded.
 */
function runAllTests(loadSuccess, responseCallback) {
  var response = { type: 'processTestResults',
                   testResults: {error: 0} };
  if (loadSuccess) {
    try {
      var messageListener = function(message) {
        // Get the test result.
        nacl_module.removeEventListener('message', messageListener, false);
        response.testResults.testResult = message.data;

        // Send the test result back to the main html page to check.
        // The main page will make sure the nexe loaded and that the
        // TestSimple test passed.
        responseCallback(response);
      };

      // Kick off the test.  The test will respond via post message.
      nacl_module.addEventListener("message", messageListener, false);
      nacl_module.postMessage("TestSimple");
    } catch (err) {
      // Trying to kick off the test failed, and threw a Javascript
      // exception.  Send the exception back to the main page so it can be
      // displayed.
      response.testResults.error = err.toString();
      responseCallback(response);
    }
  } else {
    // Notify the main page the nexe did not load.
    response.testResults.error = 'NaCl module failed to load';
    responseCallback(response);
  }
}

/**
 * This function is run from the embeding page, via the sendRequest()
 * mechanism in chrome.extension.
 * @return An object whose keys are the test ids, and values are the test
 *     results.
 */
function loadModuleAndRunAllTests(responseCallback) {
  var nacl_module = document.getElementById('nacl_module');
  waitForNaClModule(nacl_module, runAllTests, responseCallback);
}

/**
 * Handle the 'runTests' RPC request.  This request is sent from the
 * test_bridge script.  For more info, see:
 *     http://code.google.com/chrome/extensions/messaging.html
 */
// To deal with a race condition in Chrome extensions, the bridge script
// will try to initiate multiple connections.  Only respond to the first
// one we see.
var connected = false;
chrome.extension.onConnect.addListener(function(port) {
  if(!connected) {
    connected = true;
    loadModuleAndRunAllTests(function(response) {
      port.postMessage(response);
    });
  }
});
