// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tab where the content script has been injected.
var testTabId;

chrome.test.getConfig(function(config) {

  function rewriteURL(url) {
    var isFtp = /^ftp:/i.test(url);
    var port = isFtp ? config.ftpServer.port : config.testServer.port;
    return url.replace(/PORT/, port);
  }

  function doReq(domain, expectSuccess) {
    var url = rewriteURL(domain + ':PORT/extensions/test_file.txt');

    chrome.tabs.sendRequest(testTabId, url, function(response) {
      if (response.thrownError) {
        chrome.test.fail(response.thrownError);
        return;
      }
      if (expectSuccess) {
        chrome.test.assertEq('load', response.event);
        if (/^https?:/i.test(url))
          chrome.test.assertEq(200, response.status);
        chrome.test.assertEq('Hello!', response.text);
      } else {
        chrome.test.assertEq('error', response.event);
        chrome.test.assertEq(0, response.status);
      }

      chrome.test.succeed();
    });
  }

  chrome.tabs.create({
      url: rewriteURL('http://localhost:PORT/extensions/test_file.html')},
      function(tab) {
        testTabId = tab.id;
      });

  chrome.extension.onRequest.addListener(function(message) {
    chrome.test.assertEq('injected', message);

    chrome.test.runTests([
      function allowedOrigin() {
        doReq('http://a.com', true);
      },
      function diallowedOrigin() {
        doReq('http://c.com', false);
      },
      function allowedSubdomain() {
        doReq('http://foo.b.com', true);
      },
      function noSubdomain() {
        doReq('http://b.com', true);
      },
      function disallowedSubdomain() {
        doReq('http://foob.com', false);
      },
      // TODO(asargent): Explicitly create SSL test server and enable the test.
      // function disallowedSSL() {
      //   doReq('https://a.com', false);
      // },
      function targetPageAlwaysAllowed() {
        // Even though localhost does not show up in the host permissions, we
        // can still make requests to it since it's the page that the content
        // script is injected into.
        doReq('http://localhost', true);
      },
      function allowedFtpHostDisllowed() {
        doReq('ftp://127.0.0.1', false);
      },
      function disallowedFtpHostDisallowed() {
        // The host is the same as the current page, but the scheme differs.
        // The origin is not whitelisted, so the same origin policy must kick in
        // and block the request.
        doReq('ftp://localhost', false);
      }
    ]);
  });
});
