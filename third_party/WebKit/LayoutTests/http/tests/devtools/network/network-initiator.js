// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests resources initiator for images initiated by IMG tag, static CSS, CSS class added from JavaScript and XHR.\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('network');
  await TestRunner.evaluateInPagePromise(`
      function loadData()
      {
          var iframe = document.createElement("iframe");
          iframe.src = "resources/network-initiator-frame.html";
          document.body.appendChild(iframe);
      }
  `);

  step2();

  function step2() {
    ConsoleTestRunner.addConsoleSniffer(step3, true);
    TestRunner.evaluateInPage('loadData()');
  }

  var awaitedMessages = 2;
  function step3() {
    --awaitedMessages;
    if (awaitedMessages > 0)
      return;
    function dumpInitiator(url) {
      var matching_requests = NetworkTestRunner.findRequestsByURLPattern(new RegExp(url.replace('.', '\\.')));
      if (matching_requests.length === 0) {
        TestRunner.addResult(url + ' NOT FOUND');
        return;
      }
      var request = matching_requests[0];
      var initiator = request.initiator();
      TestRunner.addResult(request.url() + ': ' + initiator.type);
      if (initiator.url)
        TestRunner.addResult('    ' + initiator.url + ' ' + initiator.lineNumber);
      if (initiator.stack) {
        var stackTrace = initiator.stack;
        for (var i = 0; i < stackTrace.callFrames.length; ++i) {
          var frame = stackTrace.callFrames[i];
          if (frame.lineNumber) {
            TestRunner.addResult('    ' + frame.functionName + ' ' + frame.url + ' ' + frame.lineNumber);
            break;
          }
        }
      }
    }

    dumpInitiator('initiator.css');
    dumpInitiator('size=100');
    dumpInitiator('size=400');
    dumpInitiator('style.css');
    dumpInitiator('empty.html');
    dumpInitiator('module1.js');
    dumpInitiator('module2.js');
    TestRunner.completeTest();
  }
})();
