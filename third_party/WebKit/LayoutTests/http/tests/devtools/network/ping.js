// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests that hyperlink auditing (ping) requests appear in network panel')
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('network');
  await TestRunner.loadHTML(`<a id="pingLink" href="#" ping="ping.js">ping</a>`);
  await TestRunner.evaluateInPagePromise(`
      testRunner.overridePreference("WebKitHyperlinkAuditingEnabled", 1);
      function navigateLink()
      {
          var evt = document.createEvent("MouseEvents");
          evt.initMouseEvent("click");
          var link = document.getElementById("pingLink");
          link.dispatchEvent(evt);
      }
  `);

  TestRunner.evaluateInPage('navigateLink()');
  await TestRunner.addSnifferPromise(SDK.NetworkDispatcher.prototype, 'requestWillBeSent');
  var request = NetworkTestRunner.networkRequests().peekLast();
  if (request.url().endsWith('/')) {
    await TestRunner.addSnifferPromise(SDK.NetworkDispatcher.prototype, 'requestWillBeSent');
    request = NetworkTestRunner.networkRequests().pop();
  }

  TestRunner.addResult(request.url());
  TestRunner.addResult('resource.requestContentType: ' + request.requestContentType());

  TestRunner.completeTest();
})();
