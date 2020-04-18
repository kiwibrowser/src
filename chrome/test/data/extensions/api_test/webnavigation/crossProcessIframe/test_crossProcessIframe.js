// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

onload = function() {
  debug = true;
  var getURL = chrome.extension.getURL;
  var URL_MAIN = getURL('main.html');
  var URL_FRAME1 = 'http://a.com:PORT/extensions/api_test/webnavigation/' +
      'crossProcessIframe/frame.html';
  var URL_FRAME2 = 'http://b.com:PORT/extensions/api_test/webnavigation/' +
      'crossProcessIframe/frame.html';
  var URL_FRAME3 = 'http://c.com:PORT/extensions/api_test/webnavigation/' +
      'crossProcessIframe/frame.html';
  chrome.tabs.create({'url': 'about:blank'}, function(tab) {
    var tabId = tab.id;
    chrome.test.getConfig(function(config) {
      var fixPort = function(url) {
        return url.replace(/PORT/g, config.testServer.port);
      };
      URL_FRAME1 = fixPort(URL_FRAME1);
      URL_FRAME2 = fixPort(URL_FRAME2);
      URL_FRAME3 = fixPort(URL_FRAME3);

      chrome.test.runTests([
        // Navigates from an extension page to a HTTP page which causes a
        // process switch. The extension page embeds a same-process iframe which
        // embeds another frame that navigates three times (cross-process):
        // c. Loaded by the parent frame.
        // d. Navigated by the parent frame.
        // e. Navigated by the child frame.
        // Tests whether the frameId stays constant across navigations.
        function crossProcessIframe() {
          expect([
            { label: 'main-onBeforeNavigate',
              event: 'onBeforeNavigate',
              details: { frameId: 0,
                         parentFrameId: -1,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_MAIN }},
            { label: 'main-onCommitted',
              event: 'onCommitted',
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: 'link',
                         url: URL_MAIN }},
            { label: 'main-onDOMContentLoaded',
              event: 'onDOMContentLoaded',
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_MAIN }},
            { label: 'main-onCompleted',
              event: 'onCompleted',
              details: { frameId: 0,
                         processId: 0,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_MAIN }},
            { label: 'a.com-onBeforeNavigate',
              event: 'onBeforeNavigate',
              details: { frameId: 1,
                         parentFrameId: 0,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_FRAME1 }},
            { label: 'a.com-onCommitted',
              event: 'onCommitted',
              details: { frameId: 1,
                         processId: 1,
                         tabId: 0,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: 'auto_subframe',
                         url: URL_FRAME1 }},
            { label: 'a.com-onDOMContentLoaded',
              event: 'onDOMContentLoaded',
              details: { frameId: 1,
                         processId: 1,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_FRAME1 }},
            { label: 'a.com-onCompleted',
              event: 'onCompleted',
              details: { frameId: 1,
                         processId: 1,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_FRAME1 }},
            { label: 'b.com-onBeforeNavigate',
              event: 'onBeforeNavigate',
              details: { frameId: 1,
                         parentFrameId: 0,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_FRAME2 }},
            { label: 'b.com-onCommitted',
              event: 'onCommitted',
              details: { frameId: 1,
                         processId: 2,
                         tabId: 0,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: 'manual_subframe',
                         url: URL_FRAME2 }},
            { label: 'b.com-onDOMContentLoaded',
              event: 'onDOMContentLoaded',
              details: { frameId: 1,
                         processId: 2,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_FRAME2 }},
            { label: 'b.com-onCompleted',
              event: 'onCompleted',
              details: { frameId: 1,
                         processId: 2,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_FRAME2 }},
            { label: 'c.com-onBeforeNavigate',
              event: 'onBeforeNavigate',
              details: { frameId: 1,
                         parentFrameId: 0,
                         processId: -1,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_FRAME3 }},
            { label: 'c.com-onCommitted',
              event: 'onCommitted',
              details: { frameId: 1,
                         processId: 3,
                         tabId: 0,
                         timeStamp: 0,
                         transitionQualifiers: [],
                         transitionType: 'manual_subframe',
                         url: URL_FRAME3 }},
            { label: 'c.com-onDOMContentLoaded',
              event: 'onDOMContentLoaded',
              details: { frameId: 1,
                         processId: 3,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_FRAME3 }},
            { label: 'c.com-onCompleted',
              event: 'onCompleted',
              details: { frameId: 1,
                         processId: 3,
                         tabId: 0,
                         timeStamp: 0,
                         url: URL_FRAME3 }}],
            [
              navigationOrder('main-'),
              navigationOrder('a.com-'),
              navigationOrder('b.com-'),
              navigationOrder('c.com-'),
              ['a.com-onCommitted', 'b.com-onBeforeNavigate'],
              ['b.com-onCommitted', 'c.com-onBeforeNavigate'],
            ]);

          chrome.tabs.update(tabId, {
            url: URL_MAIN + '?' + config.testServer.port
          });
        },

      ]);
    });
  });
};
