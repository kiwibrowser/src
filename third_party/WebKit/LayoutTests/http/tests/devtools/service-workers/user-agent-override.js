// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that User-Agent override works for requests from Service Workers.\n`);
  await TestRunner.loadModule('application_test_runner');
    // Note: every test that uses a storage API must manually clean-up state from previous tests.
  await ApplicationTestRunner.resetState();

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('resources');

  function waitForTarget() {
    return new Promise(function(resolve) {
      var sniffer = {
        targetAdded: function(target) {
          if (TestRunner.isServiceWorker(target)) {
            resolve();
            SDK.targetManager.unobserveTargets(sniffer);
          }
        },

        targetRemoved: function(e) {}
      };
      SDK.targetManager.observeTargets(sniffer);
    });
  }

  function waitForConsoleMessage(regex) {
    return new Promise(function(resolve) {
      SDK.consoleModel.addEventListener(SDK.ConsoleModel.Events.MessageAdded, sniff);

      function sniff(e) {
        if (e.data && regex.test(e.data.messageText)) {
          resolve(e.data);
          SDK.consoleModel.removeEventListener(SDK.ConsoleModel.Events.MessageAdded, sniff);
        }
      }
    });
  }

  var scriptURL = 'http://127.0.0.1:8000/devtools/service-workers/resources/user-agent-override-worker.js';
  var scope = 'http://127.0.0.1:8000/devtools/service-workers/resources/user-agent-override/';
  var userAgentString = 'Mozilla/5.0 (Overridden User Agent)';
  var originalUserAgent = navigator.userAgent;

  TestRunner.addResult('Enable emulation and set User-Agent override');
  SDK.multitargetNetworkManager.setUserAgentOverride(userAgentString);

  ApplicationTestRunner.registerServiceWorker(scriptURL, scope)
      .then(waitForTarget)
      .then(ApplicationTestRunner.postToServiceWorker.bind(ApplicationTestRunner, scope, 'message'))
      .then(waitForConsoleMessage.bind(null, /HTTP_USER_AGENT/))
      .then(function(msg) {
        TestRunner.addResult('Overriden user agent: ' + msg.messageText);
        TestRunner.addResult('Disable emulation');
        SDK.multitargetNetworkManager.setUserAgentOverride('');
        return ApplicationTestRunner.unregisterServiceWorker(scope);
      })
      .then(function() {
        return ApplicationTestRunner.registerServiceWorker(scriptURL + '?2', scope);
      })
      .then(waitForTarget)
      .then(ApplicationTestRunner.postToServiceWorker.bind(ApplicationTestRunner, scope, 'message'))
      .then(waitForConsoleMessage.bind(null, /HTTP_USER_AGENT/))
      .then(function(msg) {
        TestRunner.addResult('User agent without override is correct: ' + (msg.messageText != userAgentString));
        return ApplicationTestRunner.unregisterServiceWorker(scope);
      })
      .then(function() {
        TestRunner.addResult('Test complete');
        TestRunner.completeTest();
      })
      .catch(function(err) {
        console.log(err);
        TestRunner.completeTest();
      });
})();
