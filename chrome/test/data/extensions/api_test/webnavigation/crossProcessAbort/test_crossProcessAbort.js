// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

onload = function() {
  var getURL = chrome.extension.getURL;
  var INITIAL_URL = getURL("initial.html");
  var SAME_SITE_URL = getURL("empty.html");
  var CROSS_SITE_URL = "http://127.0.0.1:PORT/title1.html";
  chrome.tabs.create({"url": "about:blank"}, function(tab) {
    var tabId = tab.id;
    chrome.test.getConfig(function(config) {
      var fixPort = function(url) {
        return url.replace(/PORT/g, config.testServer.port);
      };
      CROSS_SITE_URL = fixPort(CROSS_SITE_URL);

      chrome.test.runTests([
        // Navigates to a slow cross-site URL (extension to HTTP) and starts
        // a slow renderer-initiated, non-user, same-site navigation.
        // The cross-site navigation commits while the same-site navigation
        // is in process and this test expects an error event for the
        // same-site navigation.
        function crossProcessAbort() {
          expect([
            { label: "a-onBeforeNavigate",
              event: "onBeforeNavigate",
              details: { frameId: 0,
                         parentFrameId: -1,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: INITIAL_URL }},
            { label: "a-onCommitted",
              event: "onCommitted",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: "link",
                         url: INITIAL_URL }},
            { label: "a-onDOMContentLoaded",
              event: "onDOMContentLoaded",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         url: INITIAL_URL }},
            { label: "a-onCompleted",
              event: "onCompleted",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         url: INITIAL_URL }},
            { label: "b-onBeforeNavigate",
              event: "onBeforeNavigate",
              details: { frameId: 0,
                         parentFrameId: -1,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: CROSS_SITE_URL }},
            { label: "b-onCommitted",
              event: "onCommitted",
              details: { frameId: 0,
                         processId: 1,
                         tabId: 0,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: "link",
                         url: CROSS_SITE_URL }},
            { label: "b-onDOMContentLoaded",
              event: "onDOMContentLoaded",
              details: { frameId: 0,
                         processId: 1,
                         tabId: 0,
                         timeStamp: 0,
                         url: CROSS_SITE_URL }},
            { label: "b-onCompleted",
              event: "onCompleted",
              details: { frameId: 0,
                         processId: 1,
                         tabId: 0,
                         timeStamp: 0,
                         url: CROSS_SITE_URL }},
            { label: "c-onBeforeNavigate",
              event: "onBeforeNavigate",
              details: { frameId: 0,
                         parentFrameId: -1,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: SAME_SITE_URL }},
            { label: "c-onErrorOccurred",
              event: "onErrorOccurred",
              details: { error: "net::ERR_ABORTED",
                         frameId: 0,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: SAME_SITE_URL }}
           ],
            [ navigationOrder("a-"),
              navigationOrder("b-"),
              [ "a-onCompleted",
                "b-onBeforeNavigate",
                "c-onBeforeNavigate",
                "c-onErrorOccurred",
                "b-onCommitted" ]]);

          chrome.tabs.update(
              tabId,
              { url: getURL('initial.html?' + config.testServer.port +
                            '/title1.html') })
        },
      ]);
    });
  });
};
