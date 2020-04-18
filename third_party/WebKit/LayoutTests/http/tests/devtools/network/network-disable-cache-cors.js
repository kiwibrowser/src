// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that preflight OPTIONS is always sent if 'Disable cache' is checked, and that network instrumentation does not produce errors for redirected preflights.\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('network');
  await TestRunner.evaluateInPagePromise(`
      function sendXHR(callback)
      {
          var xhr = new XMLHttpRequest();
          xhr.onreadystatechange = function()
          {
              if (xhr.readyState === XMLHttpRequest.DONE)
                  callback();
          };
          xhr.open("POST", "http://localhost:8000/devtools/network/resources/cors.cgi");
          xhr.setRequestHeader("Content-Type", "application/xml");
          xhr.send("<xml></xml>");
      }


      function step1()
      {
          sendXHR(step2); // Should issue OPTIONS and POST requests.
      }

      function step2()
      {
          sendXHR(step3); // Should issue OPTIONS and POST requests.
      }

      function step3()
      {
          console.log("Done.");
      }
  `);

  NetworkTestRunner.makeFetch(
      'http://localhost:8080/devtools/network/resources/cors-redirect.cgi', {headers: {'x-test': 'redirect'}},
      function() {});
  NetworkTestRunner.makeFetch('http://localhost:8080/devtools/network/resources/cors-redirect.cgi', {}, disableCache);

  async function disableCache() {
    await TestRunner.NetworkAgent.setCacheDisabled(true);
    NetworkTestRunner.recordNetwork();
    ConsoleTestRunner.addConsoleSniffer(step4);
    TestRunner.evaluateInPage('step1();');
  }

  async function step4() {
    await TestRunner.NetworkAgent.setCacheDisabled(false);
    var requests = NetworkTestRunner.networkRequests();
    for (var i = 0; i < requests.length; ++i) {
      var request = requests[i];
      var method = request.requestMethod;
      if (method === 'OPTIONS' || method === 'POST')
        TestRunner.addResult(method + ': ' + request.url());
    }
    TestRunner.completeTest();
  }
})();
