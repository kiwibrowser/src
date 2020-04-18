// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

onload = function() {
  var URL_START =
      'http://127.0.0.1:PORT/extensions/api_test/webnavigation/download/a.html';
  var URL_LOAD_REDIRECT = "http://127.0.0.1:PORT/server-redirect";
  var URL_NOT_FOUND = "http://127.0.0.1:PORT/not-found";
  chrome.tabs.create({"url": "about:blank"}, function(tab) {
    var tabId = tab.id;
    chrome.test.getConfig(function(config) {
      var fixPort = function(url) {
        return url.replace(/PORT/g, config.testServer.port);
      };
      URL_START = fixPort(URL_START);
      URL_LOAD_REDIRECT = fixPort(URL_LOAD_REDIRECT);
      URL_NOT_FOUND = fixPort(URL_NOT_FOUND);
      chrome.test.runTests([
        // Navigates to a page that redirects (on the server side) to a.html.
        function download() {
          expect([
            { label: "a-onBeforeNavigate",
              event: "onBeforeNavigate",
              details: { frameId: 0,
                         parentFrameId: -1,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_START }},
            { label: "a-onCommitted",
              event: "onCommitted",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: "link",
                         url: URL_START }},
            { label: "a-onDOMContentLoaded",
              event: "onDOMContentLoaded",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_START }},
            { label: "a-onCompleted",
              event: "onCompleted",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_START }}],
          [ navigationOrder("a-") ]);
          chrome.tabs.update(
              tabId, { url: URL_START + "?" + config.testServer.port });
        },
      ]);
    });
  });
};
