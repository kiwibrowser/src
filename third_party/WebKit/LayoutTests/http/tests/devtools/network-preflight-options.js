// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that preflight OPTIONS requests appear in Network resources\n`);

  await TestRunner.evaluateInPagePromise(`
      function sendXHR(url, forcePreflight, async, callback)
      {
          var xhr = new XMLHttpRequest();

          xhr.onreadystatechange = function()
          {
              if (xhr.readyState === XMLHttpRequest.DONE) {
                  if (typeof(callback) === "function")
                      callback();
              }
          };

          xhr.open("POST", url, async);
          xhr.setRequestHeader("Content-Type", forcePreflight ? "application/xml" : "text/plain");
          try {
              xhr.send("<xml></xml>");  // Denied requests will cause exceptions.
          } catch (e) {
          }
      }

      var baseTargetURL = "http://localhost:8000/devtools/resources/cors-target.php";

      function targetURL(id, deny, addTimestamp)
      {
          var result = baseTargetURL + "?id=" + id;
          if (deny)
              result += "&deny=yes";
          if (addTimestamp)
              result += "&date=" + Date.now();
          return result;
      }

      function doCrossOriginXHR()
      {
          // Failed POSTs with no preflight check should result in a POST request being logged
          sendXHR(targetURL(0, true, false), false, false);
          // Failed POSTs with preflight check should result in an OPTIONS request being logged
          sendXHR(targetURL(1, true, false), true, false);
          // Successful POSTs with preflight check should result in an OPTIONS request followed by POST request being logged
          // Generate request name based on timestamp to defeat OPTIONS request caching (this is only relevant for repeated invocations of the test in signle instance of DRT)
          sendXHR(targetURL(2, false, true), true, false);

          // And now send the same requests asynchronously
          // Add redundant async parameter to ensure this request differs from the one above.
          sendXHR(targetURL(3, true, false), false, true, step2);
      }

      function step2()
      {
          sendXHR(targetURL(4, true, false), true, true, step3);
      }

      function step3()
      {
          sendXHR(targetURL(5, false, true), true, true);
      }
  `);

  var requestMessages = [];
  var postRequestsCount = 0;
  function onRequest(event) {
    var request = event.data;
    var idMatch = /\?id=([0-9]*)&/.exec(request.url());
    var requestId = idMatch[1];
    var requestMessage = requestId + ' ' + request.resourceType() + ':' + request.requestMethod + ' ' +
        request.url().replace(/[&?]date=\d+/, '');
    requestMessages.push(requestMessage);
    if (request.requestMethod === 'POST' && ++postRequestsCount === 4) {
      requestMessages.sort();
      for (var i = 0; i < requestMessages.length; i++)
        TestRunner.addResult(requestMessages[i]);
      TestRunner.completeTest();
    }
  }
  TestRunner.networkManager.addEventListener(SDK.NetworkManager.Events.RequestFinished, onRequest);
  TestRunner.evaluateInPage('doCrossOriginXHR();');
})();
