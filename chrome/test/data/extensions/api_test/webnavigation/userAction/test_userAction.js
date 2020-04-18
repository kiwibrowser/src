// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

onload = function() {
  var getURL = chrome.extension.getURL;
  var URL_MAIN = getURL("a.html");
  var SUBFRAME_URL =
          "http://127.0.0.1:PORT/extensions/api_test/webnavigation/userAction/subframe.html";

  chrome.tabs.create({"url": "about:blank"}, function(tab) {
    var tabId = tab.id;

    chrome.test.getConfig(function(config) {
      var fixPort = function(url) {
        return url.replace(/PORT/g, config.testServer.port);
      };
      SUBFRAME_URL = fixPort(SUBFRAME_URL);

      chrome.test.runTests([
        // Opens a tab and waits for the user to click on a link in it.
        function userAction() {
          expect([
            { label: "a-onBeforeNavigate",
              event: "onBeforeNavigate",
              details: { frameId: 0,
                         parentFrameId: -1,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_MAIN }},
            { label: "a-onCommitted",
              event: "onCommitted",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: "typed",
                         url: URL_MAIN }},
            { label: "a-onDOMContentLoaded",
              event: "onDOMContentLoaded",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_MAIN }},
            { label: "a-onCompleted",
              event: "onCompleted",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_MAIN }},

            { label: "subframe-onBeforeNavigate",
              event: "onBeforeNavigate",
              details: { frameId: 1,
                         parentFrameId: 0,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: SUBFRAME_URL }},
            { label: "subframe-onCommitted",
              event: "onCommitted",
              details: { frameId: 1,
                         processId: 1,
                         tabId: 0,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: "auto_subframe",
                         url: SUBFRAME_URL }},
            { label: "subframe-onDOMContentLoaded",
              event: "onDOMContentLoaded",
              details: { frameId: 1,
                         processId: 1,
                         tabId: 0,
                         timeStamp: 0,
                         url: SUBFRAME_URL }},
            { label: "subframe-onCompleted",
              event: "onCompleted",
              details: { frameId: 1,
                         processId: 1,
                         tabId: 0,
                         timeStamp: 0,
                         url: SUBFRAME_URL }},

            { label: "b-onCreatedNavigationTarget",
              event: "onCreatedNavigationTarget",
              details: { sourceFrameId: 1,
                         sourceProcessId: 0,
                         sourceTabId: 0,
                         tabId: 1,
                         timeStamp: 0,
                         url: getURL('b.html') }},
            { label: "b-onBeforeNavigate",
              event: "onBeforeNavigate",
              details: { frameId: 0,
                         parentFrameId: -1,
                         processId: -1,
                         tabId: 1,
                         timeStamp: 0,
                         url: getURL('b.html') }},
            { label: "b-onCommitted",
              event: "onCommitted",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 1,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: "link",
                         url: getURL('b.html') }},
            { label: "b-onDOMContentLoaded",
              event: "onDOMContentLoaded",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 1,
                         timeStamp: 0,
                         url: getURL('b.html') }},
            { label: "b-onCompleted",
              event: "onCompleted",
              details: { frameId: 0,
                         processId: 0,
                         tabId: 1,
                         timeStamp: 0,
                         url: getURL('b.html') }}],
            [ navigationOrder("a-"),
              navigationOrder("subframe-"),
              navigationOrder("b-"),
              [ "a-onCompleted",
                "b-onCreatedNavigationTarget",
                "b-onBeforeNavigate" ]]);

          // Notify the api test that we're waiting for the user.
          chrome.test.notifyPass();
        },
      ]);
    });
  });
};
