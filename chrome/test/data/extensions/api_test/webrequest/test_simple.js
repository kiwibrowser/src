// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Constants as functions, not to be called until after runTests.
function getURLHttpSimpleLoad() {
  return getServerURL('extensions/api_test/webrequest/simpleLoad/a.html');
}

function getURLHttpSimpleLoadRedirect() {
  return getServerURL('server-redirect?'+getURLHttpSimpleLoad());
}

runTests([
  // Navigates to a blank page.
  function simpleLoad() {
    expect(
      [  // events
        { label: "a-onBeforeRequest",
          event: "onBeforeRequest",
          details: {
            url: getURL("simpleLoad/a.html"),
            frameUrl: getURL("simpleLoad/a.html"),
            initiator: getDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "a-onResponseStarted",
          event: "onResponseStarted",
          details: {
            url: getURL("simpleLoad/a.html"),
            statusCode: 200,
            fromCache: false,
            statusLine: "HTTP/1.1 200 OK",
            initiator: getDomain(initiators.BROWSER_INITIATED),
            // Request to chrome-extension:// url has no IP.
          }
        },
        { label: "a-onCompleted",
          event: "onCompleted",
          details: {
            url: getURL("simpleLoad/a.html"),
            statusCode: 200,
            fromCache: false,
            statusLine: "HTTP/1.1 200 OK",
            initiator: getDomain(initiators.BROWSER_INITIATED),
            // Request to chrome-extension:// url has no IP.
          }
        },
      ],
      [  // event order
      ["a-onBeforeRequest", "a-onResponseStarted", "a-onCompleted"] ]);
    navigateAndWait(getURL("simpleLoad/a.html"));
  },

  // Navigates to a blank page via HTTP. Only HTTP requests get the
  // onBeforeSendHeaders event.
  function simpleLoadHttp() {
    expect(
      [  // events
        { label: "onBeforeRequest-1",
          event: "onBeforeRequest",
          details: {
            url: getURLHttpSimpleLoadRedirect(),
            frameUrl: getURLHttpSimpleLoadRedirect(),
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onBeforeSendHeaders-1",
          event: "onBeforeSendHeaders",
          details: {
            url: getURLHttpSimpleLoadRedirect(),
            requestHeadersValid: true,
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onSendHeaders-1",
          event: "onSendHeaders",
          details: {
            url: getURLHttpSimpleLoadRedirect(),
            requestHeadersValid: true,
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onHeadersReceived-1",
          event: "onHeadersReceived",
          details: {
            url: getURLHttpSimpleLoadRedirect(),
            responseHeadersExist: true,
            statusLine: "HTTP/1.1 301 Moved Permanently",
            statusCode: 301,
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onBeforeRedirect",
          event: "onBeforeRedirect",
          details: {
            url: getURLHttpSimpleLoadRedirect(),
            redirectUrl: getURLHttpSimpleLoad(),
            statusCode: 301,
            responseHeadersExist: true,
            ip: "127.0.0.1",
            fromCache: false,
            statusLine: "HTTP/1.1 301 Moved Permanently",
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onBeforeRequest-2",
          event: "onBeforeRequest",
          details: {
            url: getURLHttpSimpleLoad(),
            frameUrl: getURLHttpSimpleLoad(),
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onBeforeSendHeaders-2",
          event: "onBeforeSendHeaders",
          details: {
            url: getURLHttpSimpleLoad(),
            requestHeadersValid: true,
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onSendHeaders-2",
          event: "onSendHeaders",
          details: {
            url: getURLHttpSimpleLoad(),
            requestHeadersValid: true,
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onHeadersReceived-2",
          event: "onHeadersReceived",
          details: {
            url: getURLHttpSimpleLoad(),
            responseHeadersExist: true,
            statusLine: "HTTP/1.1 200 OK",
            statusCode: 200,
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onResponseStarted",
          event: "onResponseStarted",
          details: {
            url: getURLHttpSimpleLoad(),
            statusCode: 200,
            responseHeadersExist: true,
            ip: "127.0.0.1",
            fromCache: false,
            statusLine: "HTTP/1.1 200 OK",
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onCompleted",
          event: "onCompleted",
          details: {
            url: getURLHttpSimpleLoad(),
            statusCode: 200,
            ip: "127.0.0.1",
            fromCache: false,
            responseHeadersExist: true,
            statusLine: "HTTP/1.1 200 OK",
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
          }
        }
      ],
      [  // event order
        ["onBeforeRequest-1", "onBeforeSendHeaders-1", "onSendHeaders-1",
         "onHeadersReceived-1", "onBeforeRedirect",
         "onBeforeRequest-2", "onBeforeSendHeaders-2", "onSendHeaders-2",
         "onHeadersReceived-2", "onResponseStarted", "onCompleted"] ],
      {urls: ["<all_urls>"]},  // filter
      ["requestHeaders", "responseHeaders"]);
    navigateAndWait(getURLHttpSimpleLoadRedirect());
  },

  // Navigates to a non-existing page.
  function nonExistingLoad() {
    expect(
      [  // events
        { label: "onBeforeRequest",
          event: "onBeforeRequest",
          details: {
            url: getURL("does_not_exist.html"),
            frameUrl: getURL("does_not_exist.html"),
            initiator: getDomain(initiators.BROWSER_INITIATED),
          }
        },
        { label: "onErrorOccurred",
          event: "onErrorOccurred",
          details: {
            url: getURL("does_not_exist.html"),
            fromCache: false,
            error: "net::ERR_FILE_NOT_FOUND",
            initiator: getDomain(initiators.BROWSER_INITIATED),
            // Request to chrome-extension:// url has no IP.
          }
        },
      ],
      [  // event order
        ["onBeforeRequest", "onErrorOccurred"] ]);
    navigateAndWait(getURL("does_not_exist.html"));
  },
]);
