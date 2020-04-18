// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tab where the content script has been injected.
var testTabId;

var pass = chrome.test.callbackPass;

chrome.test.getConfig(function(config) {

  function rewriteURL(url) {
    return url.replace(/PORT/, config.testServer.port);
  }

  function doReq(domain, expectSuccess) {
    var url = rewriteURL(domain + ':PORT/extensions/test_file.txt');

    chrome.tabs.sendRequest(testTabId, url, pass(function(response) {
      if (expectSuccess) {
        chrome.test.assertEq('load', response.event);
        chrome.test.assertEq(200, response.status);
        chrome.test.assertEq('Hello!', response.text);
      } else {
        chrome.test.assertEq(0, response.status);
      }
    }));
  }

  chrome.tabs.create({
      url: rewriteURL('http://localhost:PORT/extensions/test_file.html')},
      function(tab) {
        testTabId = tab.id;
      });

  chrome.extension.onRequest.addListener(function(message) {
    chrome.test.assertEq('injected', message);

    chrome.test.runTests([
      function domainOne() {
        doReq('http://a.com', true);
      },
      function domainTwo() {
        doReq('http://c.com', true);
      }
    ]);
  });
});
