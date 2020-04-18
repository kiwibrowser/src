// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests XSL stylsheet content. http://crbug.com/603806\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('network');
  await TestRunner.evaluateInPagePromise(`
      function loadIframe()
      {
          var iframe = document.createElement("iframe");
          document.body.appendChild(iframe);
          iframe.src = "resources/xml-with-stylesheet.xml";
      }
  `);

  var expectedResourceCount = 2;
  var foundResources = 0;
  var resultsOutput = [];
  NetworkTestRunner.recordNetwork();
  TestRunner.evaluateInPage('loadIframe()');
  TestRunner.addSniffer(SDK.NetworkDispatcher.prototype, 'loadingFinished', loadingFinished, true);

  function loadingFinished(requestId) {
    var request = BrowserSDK.networkLog.requestByManagerAndId(TestRunner.networkManager, requestId);
    request.requestContent().then(contentReceived.bind(this, request));
  }
  function contentReceived(request, content) {
    var output = [];
    output.push(request.url());
    output.push('resource.type: ' + request.resourceType());
    output.push('resource.content: ' + content);

    resultsOutput.push(output.join('\n'));
    if (++foundResources >= expectedResourceCount)
      finish();
  }
  function finish() {
    TestRunner.addResult(resultsOutput.sort().join('\n'));
    TestRunner.completeTest();
  }
})();
