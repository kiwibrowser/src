// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that XHRs with the same url as a main resource have correct category.\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('network');

  await TestRunner.evaluateInPagePromise(`
      function loadIframe()
      {
          iframe = document.createElement("iframe");
          document.body.appendChild(iframe);
          iframe.src = "resources/resource.php";
          console.log("iframe loaded");
      }
  `);

  NetworkTestRunner.recordNetwork();
  ConsoleTestRunner.addConsoleSniffer(step2);
  TestRunner.evaluateInPage('loadIframe()');

  function step2() {
    NetworkTestRunner.makeSimpleXHR('GET', 'resources/resource.php', true, () => {
      TestRunner.deprecatedRunAfterPendingDispatches(step3);
    });
  }

  function step3() {
    var request1 = NetworkTestRunner.networkRequests().pop();
    TestRunner.addResult(request1.resourceType().name());
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  }
})();
