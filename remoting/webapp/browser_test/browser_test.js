// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 *
 * Provides basic functionality for JavaScript based browser test.
 *
 * To define a browser test, create a class under the browserTest namespace.
 * You can pass arbitrary object literals to the browser test from the C++ test
 * harness as the test data.  Each browser test class should implement the run
 * method.
 * For example:
 *
 * browserTest.My_Test = function() {};
 * browserTest.My_Test.prototype.run(myObjectLiteral) = function() { ... };
 *
 * The browser test is async in nature.  It will keep running until
 * browserTest.fail("My error message.") or browserTest.pass() is called.
 *
 * For example:
 *
 * browserTest.My_Test.prototype.run(myObjectLiteral) = function() {
 *   window.setTimeout(function() {
 *     if (doSomething(myObjectLiteral)) {
 *       browserTest.pass();
 *     } else {
 *       browserTest.fail('My error message.');
 *     }
 *   }, 1000);
 * };
 *
 * You will then invoke the test in C++ by calling:
 *
 *   RunJavaScriptTest(web_content, "My_Test", "{"
 *    "pin: '123123'"
 *  "}");
 */

'use strict';

/** @suppress {duplicate} */
var browserTest = browserTest || {};

/** @type {window.DomAutomationController} */
browserTest.automationController_ = null;

/**
 * @return {void}
 * @suppress {checkTypes|reportUnknownTypes}
 */
browserTest.init = function() {
  // The domAutomationController is used to communicate progress back to the
  // C++ calling code.  It will only exist if chrome is run with the flag
  // --dom-automation.  It is stubbed out here so that browser test can be run
  // under the regular app.
  if (window.domAutomationController) {
    /** @type {window.DomAutomationController} */
    browserTest.automationController_ = window.domAutomationController;
  } else {
    browserTest.automationController_ = {
      send: function(json) {
        var result = JSON.parse(json);
        if (result.succeeded) {
          console.log('Test Passed.');
        } else {
          console.error('Test Failed.\n' +
              result.error_message + '\n' + result.stack_trace);
        }
      }
    };
  };
};

/**
 * Fails the C++ calling browser test with |message| if |expr| is false.
 * @param {*} expr
 * @param {string=} opt_message
 * @return {void}
 */
browserTest.expect = function(expr, opt_message) {
  if (!expr) {
    var message = (opt_message) ? '<' + opt_message + '>' : '';
    browserTest.fail('Expectation failed.' + opt_message);
  }
};

/**
 * @param {string|Error} error
 * @return {void}
 */
browserTest.fail = function(error) {
  var error_message = error;
  var stack_trace = new base.Callstack().toString();

  if (error instanceof Error) {
    error_message = error.toString();
    stack_trace = error.stack;
  }

  console.error(error_message);

  // To run browserTest locally:
  // 1. Go to |remoting_webapp_files| and look for
  //    |remoting_webapp_js_browser_test_files| and uncomment it
  // 2. gclient runhooks
  // 3. rebuild the webapp
  // 4. Run it in the console browserTest.runTest(browserTest.MyTest, {});
  // 5. The line below will trap the test in the debugger in case of
  //    failure.
  debugger;

  browserTest.automationController_.send(JSON.stringify({
    succeeded: false,
    error_message: error_message,
    stack_trace: stack_trace
  }));
};

/**
 * @return {void}
 */
browserTest.pass = function() {
  browserTest.automationController_.send(JSON.stringify({
    succeeded: true,
    error_message: '',
    stack_trace: ''
  }));
};

/**
 * @param {string} id The id or the selector of the element.
 * @return {void}
 */
browserTest.clickOnControl = function(id) {
  var element = document.getElementById(id);
  if (!element) {
    element = document.querySelector(id);
  }
  browserTest.expect(element, 'No such element: ' + id);
  element.click();
};

/**
 * @param {remoting.AppMode} expectedMode
 * @param {number=} opt_timeout
 * @return {Promise}
 */
browserTest.onUIMode = function(expectedMode, opt_timeout) {
  if (expectedMode == remoting.currentMode) {
    // If the current mode is the same as the expected mode, return a fulfilled
    // promise.  For some reason, if we fulfill the promise in the same
    // callstack, V8 will assert at V8RecursionScope.h(66) with
    // ASSERT(!ScriptForbiddenScope::isScriptForbidden()).
    // To avoid the assert, execute the callback in a different callstack.
    return base.Promise.sleep(0);
  }

  return new Promise (function(fulfill, reject) {
    var uiModeChanged = remoting.testEvents.Names.uiModeChanged;
    var timerId = null;

    if (opt_timeout === undefined) {
      opt_timeout = browserTest.Timeout.DEFAULT;
    }

    function onTimeout() {
      remoting.testEvents.removeEventListener(uiModeChanged, onUIModeChanged);
      reject('Timeout waiting for ' + expectedMode);
    }

    /** @param {remoting.AppMode} mode */
    function onUIModeChanged(mode) {
      if (mode == expectedMode) {
        remoting.testEvents.removeEventListener(uiModeChanged, onUIModeChanged);
        window.clearTimeout(timerId);
        timerId = null;
        fulfill(true);
      }
    }

    if (opt_timeout != browserTest.Timeout.NONE) {
      timerId = window.setTimeout(onTimeout,
                                  /** @type {number} */ (opt_timeout));
    }
    remoting.testEvents.addEventListener(uiModeChanged, onUIModeChanged);
  });
};

/**
 * @return {Promise}
 */
browserTest.connectMe2Me = function() {
  var AppMode = remoting.AppMode;
  // The one second timeout is necessary because the click handler of
  // 'this-host-connect' is registered asynchronously.
  return base.Promise.sleep(1000).then(function() {
      browserTest.clickOnControl('local-host-connect-button');
    }).then(function(){
      return browserTest.onUIMode(AppMode.CLIENT_HOST_NEEDS_UPGRADE);
    }).then(function() {
      // On fulfilled.
      browserTest.clickOnControl('#host-needs-update-dialog .connect-button');
    }, function() {
      // On time out.
      return Promise.resolve();
    }).then(function() {
      return browserTest.onUIMode(AppMode.CLIENT_PIN_PROMPT, 10000);
    });
};

/**
 * @return {Promise}
 */
browserTest.disconnect = function() {
  console.assert(remoting.app instanceof remoting.DesktopRemoting,
                '|remoting.app| is not an instance of DesktopRemoting.');
  var drApp = /** @type {remoting.DesktopRemoting} */ (remoting.app);
  var mode = drApp.getConnectionMode();

  var AppMode = remoting.AppMode;
  var finishedMode = AppMode.CLIENT_SESSION_FINISHED_ME2ME;
  var finishedButton = 'client-finished-me2me-button';
  if (mode === remoting.DesktopRemoting.Mode.IT2ME) {
    finishedMode = AppMode.CLIENT_SESSION_FINISHED_IT2ME;
    finishedButton = 'client-finished-it2me-button';
  }

  var activity = remoting.app.getActivity();
  if (!activity) {
    return Promise.resolve();
  }

  activity.stop();

  return browserTest.onUIMode(finishedMode).then(function() {
    browserTest.clickOnControl(finishedButton);
    return browserTest.onUIMode(AppMode.HOME);
  });
};

/**
 * @param {string} pin
 * @param {boolean=} opt_expectError
 * @return {Promise}
 */
browserTest.enterPIN = function(pin, opt_expectError) {
  // Wait for 500ms before hitting the PIN button. From experiment, sometimes
  // the PIN prompt does not dismiss without the timeout.
  var CONNECT_PIN_WAIT = 500;

  document.getElementById('pin-entry').value = pin;

  return base.Promise.sleep(CONNECT_PIN_WAIT).then(function() {
    browserTest.clickOnControl('pin-connect-button');
  }).then(function() {
    if (opt_expectError) {
      return browserTest.expectConnectionError(
          remoting.DesktopRemoting.Mode.ME2ME,
          [remoting.Error.Tag.INVALID_ACCESS_CODE]);
    } else {
      return browserTest.expectConnected();
    }
  });
};

/**
 * @param {remoting.DesktopRemoting.Mode} connectionMode
 * @param {Array<remoting.Error.Tag>} errorTags
 * @return {Promise}
 */
browserTest.expectConnectionError = function(connectionMode, errorTags) {
  var AppMode = remoting.AppMode;
  var Timeout = browserTest.Timeout;

  // Timeout if the session is not failed within 30 seconds.
  var SESSION_CONNECTION_TIMEOUT = 30000;

  var finishButton = 'client-finished-me2me-button';
  var failureMode = AppMode.CLIENT_CONNECT_FAILED_ME2ME;

  if (connectionMode == remoting.DesktopRemoting.Mode.IT2ME) {
    finishButton = 'client-finished-it2me-button';
    failureMode = AppMode.CLIENT_CONNECT_FAILED_IT2ME;
  }

  var onConnected = browserTest.onUIMode(AppMode.IN_SESSION, Timeout.NONE);
  var onFailure = browserTest.onUIMode(failureMode, SESSION_CONNECTION_TIMEOUT);

  onConnected = onConnected.then(function() {
    return Promise.reject(
        'Expected the connection to fail.');
  });

  onFailure = onFailure.then(function() {
    /** @type {Element} */
    var errorDiv = document.getElementById('connect-error-message');
    var actual = errorDiv.innerText;
    var expected = errorTags.map(function(/** string */errorTag) {
      return l10n.getTranslationOrError(errorTag);
    });
    browserTest.clickOnControl(finishButton);

    if (expected.indexOf(actual) === -1) {
      return Promise.reject('Unexpected failure. actual: ' + actual +
                            ' expected: ' + expected.join(','));
    }
  });

  return Promise.race([onConnected, onFailure]);
};

/**
 * @return {Promise}
 */
browserTest.expectConnected = function() {
  var AppMode = remoting.AppMode;
  // Timeout if the session is not connected within 30 seconds.
  var SESSION_CONNECTION_TIMEOUT = 30000;
  var onConnected = browserTest.onUIMode(AppMode.IN_SESSION,
                                         SESSION_CONNECTION_TIMEOUT);
  var onFailure = browserTest.onUIMode(AppMode.CLIENT_CONNECT_FAILED_ME2ME,
                                       browserTest.Timeout.NONE);
  onFailure = onFailure.then(function() {
    var errorDiv = document.getElementById('connect-error-message');
    var errorMsg = errorDiv.innerText;
    return Promise.reject('Unexpected error - ' + errorMsg);
  });
  return Promise.race([onConnected, onFailure]);
};

/**
 * @param {base.EventSource} eventSource
 * @param {string} event
 * @param {number} timeoutMs
 * @param {*=} opt_expectedData
 * @return {Promise}
 */
browserTest.expectEvent = function(eventSource, event, timeoutMs,
                                   opt_expectedData) {
  return new Promise(function(fullfil, reject) {
    /** @param {string=} actualData */
    var verifyEventParameters = function(actualData) {
      if (opt_expectedData === undefined || opt_expectedData === actualData) {
        fullfil(true);
      } else {
        reject('Bad event data; expected ' + opt_expectedData +
               '; got ' + actualData);
      }
    };
    eventSource.addEventListener(event, verifyEventParameters);
    base.Promise.sleep(timeoutMs).then(function() {
      reject(Error('Event ' + event + ' not received after ' +
                   timeoutMs + 'ms.'));
    });
  });
};

/**
 * @param {Function} testClass
 * @param {*} data
 * @return {void}
 * @suppress {checkTypes|checkVars|reportUnknownTypes}
 */
browserTest.runTest = function(testClass, data) {
  try {
    var test = new testClass();
    browserTest.expect(typeof test.run == 'function');
    test.run(data);
  } catch (/** @type {Error} */ e) {
    browserTest.fail(e);
  }
};

/**
 * @param {string} newPin
 * @return {Promise}
 */
browserTest.setupPIN = function(newPin) {
  var AppMode = remoting.AppMode;
  var HOST_SETUP_WAIT = 10000;
  var Timeout = browserTest.Timeout;

  return browserTest.onUIMode(AppMode.HOST_SETUP_ASK_PIN).then(function() {
    document.getElementById('daemon-pin-entry').value = newPin;
    document.getElementById('daemon-pin-confirm').value = newPin;
    browserTest.clickOnControl('daemon-pin-ok');

    var success = browserTest.onUIMode(AppMode.HOST_SETUP_DONE, Timeout.NONE);
    var failure = browserTest.onUIMode(AppMode.HOST_SETUP_ERROR, Timeout.NONE);
    failure = failure.then(function(){
      return Promise.reject('Unexpected host setup failure');
    });
    return Promise.race([success, failure]);
  }).then(function() {
    console.log('browserTest: PIN Setup is done.');
    browserTest.clickOnControl('host-config-done-dismiss');

    // On Linux, we restart the host after changing the PIN, need to sleep
    // for ten seconds before the host is ready for connection.
    return base.Promise.sleep(HOST_SETUP_WAIT);
  });
};

/**
 * @return {Promise<boolean>}
 */
browserTest.isLocalHostStarted = function() {
  return new Promise(function(resolve) {
    remoting.hostController.getLocalHostState(function(state) {
      resolve(remoting.HostController.State.STARTED == state);
    });
  });
};

/**
 * @param {string} pin
 * @return {Promise}
 */
browserTest.ensureHostStartedWithPIN = function(pin) {
  // Return if host is already
  return browserTest.isLocalHostStarted().then(
      /** @param {boolean} started */
      function(started){
        if (!started) {
          console.log('browserTest: Enabling remote connection.');
          browserTest.clickOnControl('.start-daemon');
        } else {
          console.log('browserTest: Changing the PIN of the host to: ' +
              pin + '.');
          browserTest.clickOnControl('.change-daemon-pin');
        }
        return browserTest.setupPIN(pin);
      });
};

/**
 * Called by Browser Test in C++
 * @param {string} pin
 * @suppress {checkTypes}
 */
browserTest.ensureRemoteConnectionEnabled = function(pin) {
  browserTest.ensureHostStartedWithPIN(pin).then(function() {
    browserTest.pass();
  }, function(reason) {
    browserTest.fail(reason);
  });
};

browserTest.init();
