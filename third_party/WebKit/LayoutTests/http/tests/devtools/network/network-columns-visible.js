// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests to ensure column names are matching data.\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('network');
  await TestRunner.evaluateInPagePromise(`
      function sendXHRRequest() {
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "resources/empty.html?xhr");
          xhr.send();
      }
  `);

  var columnsToTest = [
    'name', 'method', 'status', 'protocol', 'scheme', 'domain', 'remoteaddress', 'type', 'initiator', 'cookies',
    'setcookies', 'priority', 'cache-control', 'connection', 'content-encoding', 'content-length', 'vary'
  ];

  // Setup
  NetworkTestRunner.recordNetwork();
  await NetworkTestRunner.clearNetworkCache();

  TestRunner.evaluateInPage('sendXHRRequest()');
  var request = await TestRunner.waitForEvent(
      SDK.NetworkManager.Events.RequestFinished, TestRunner.networkManager,
      request => request.name() === 'empty.html?xhr');
  var xhrNode = await NetworkTestRunner.waitForNetworkLogViewNodeForRequest(request);

  UI.panels.network._networkLogView._refresh();
  for (var columnName of columnsToTest)
    TestRunner.addResult(columnName + ': ' + xhrNode.createCell(columnName).textContent);
  TestRunner.completeTest();
})();
