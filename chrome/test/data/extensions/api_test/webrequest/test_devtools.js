// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// To debug failing tests, we need to see all observed requests in order to
// determine whether an unexpected request was received (which suggests that the
// implementation is broken) or whether an expected event was observed too early
// (which can be an indication of unreasonable test expectations
// suggests that some previous events were hidden, indicating a broken test).
// To do so, we override logAllRequests=false from framework.js.
logAllRequests = true;

// The URL that fakedevtools.html requests upon completion. This is used to
// detect when the DevTools resource has finished loading, because the requested
// front-end resources should not be visible in the webRequest API.
function getCompletionURL() {
  // We do not care about the exact events, so use a non-existing extension
  // resource because it produces only two events: onBeforeRequest and
  // onErrorOccurred.
  return getURL('does_not_exist.html');
}

// The expected events when fakedevtools.html requests to |getCompletionURL()|.
function expectCompletionEvents() {
  // These events are expected to occur. If that does not happen, check whether
  // fakedevtools.html was actually loaded via the chrome-devtools:-URL.
  expect(
      [
        {
          label: 'onBeforeRequest',
          event: 'onBeforeRequest',
          details: {
            type: 'image',
            url: getURL('does_not_exist.html'),
            frameUrl: 'unknown frame URL',
          },
        },
        {
          label: 'onErrorOccurred',
          event: 'onErrorOccurred',
          details: {
            type: 'image',
            url: getURL('does_not_exist.html'),
            fromCache: false,
            error: 'net::ERR_FILE_NOT_FOUND',
          }
        },
      ],
      [['onBeforeRequest', 'onErrorOccurred']]);
}

function expectNormalTabNavigationEvents(url) {
  var scriptUrl = new URL(url);
  var frontendHost = scriptUrl.hostname;
  scriptUrl.search = scriptUrl.hash = '';
  scriptUrl.pathname = scriptUrl.pathname.replace(/\.html$/, '.js');
  scriptUrl = scriptUrl.href;

  expect(
      [
        {
          label: 'onBeforeRequest-1',
          event: 'onBeforeRequest',
          details: {
            type: 'main_frame',
            url,
            frameUrl: url,
            initiator: getServerDomain(initiators.BROWSER_INITIATED)
          }
        },
        {
          label: 'onBeforeSendHeaders-1',
          event: 'onBeforeSendHeaders',
          details: {
            type: 'main_frame',
            url,
            initiator: getServerDomain(initiators.BROWSER_INITIATED)
          }
        },
        {
          label: 'onSendHeaders-1',
          event: 'onSendHeaders',
          details: {
            type: 'main_frame',
            url,
            initiator: getServerDomain(initiators.BROWSER_INITIATED)
          }
        },
        {
          label: 'onHeadersReceived-1',
          event: 'onHeadersReceived',
          details: {
            type: 'main_frame',
            url,
            statusLine: 'HTTP/1.1 200 OK',
            statusCode: 200,
            initiator: getServerDomain(initiators.BROWSER_INITIATED)
          }
        },
        {
          label: 'onResponseStarted-1',
          event: 'onResponseStarted',
          details: {
            type: 'main_frame',
            url,
            statusCode: 200,
            ip: '127.0.0.1',
            fromCache: false,
            statusLine: 'HTTP/1.1 200 OK',
            initiator: getServerDomain(initiators.BROWSER_INITIATED)
          }
        },
        {
          label: 'onCompleted-1',
          event: 'onCompleted',
          details: {
            type: 'main_frame',
            url,
            statusCode: 200,
            ip: '127.0.0.1',
            fromCache: false,
            statusLine: 'HTTP/1.1 200 OK',
            initiator: getServerDomain(initiators.BROWSER_INITIATED)
          }
        },
        {
          label: 'onBeforeRequest-2',
          event: 'onBeforeRequest',
          details: {
            type: 'script',
            url: scriptUrl,
            frameUrl: url,
            initiator: getServerDomain(initiators.WEB_INITIATED, frontendHost)
          }
        },
        {
          label: 'onBeforeSendHeaders-2',
          event: 'onBeforeSendHeaders',
          details: {
            type: 'script',
            url: scriptUrl,
            initiator: getServerDomain(initiators.WEB_INITIATED, frontendHost)
          }
        },
        {
          label: 'onSendHeaders-2',
          event: 'onSendHeaders',
          details: {
            type: 'script',
            url: scriptUrl,
            initiator: getServerDomain(initiators.WEB_INITIATED, frontendHost)
          }
        },
        {
          label: 'onHeadersReceived-2',
          event: 'onHeadersReceived',
          details: {
            type: 'script',
            url: scriptUrl,
            statusLine: 'HTTP/1.1 200 OK',
            statusCode: 200,
            initiator: getServerDomain(initiators.WEB_INITIATED, frontendHost)
          }
        },
        {
          label: 'onResponseStarted-2',
          event: 'onResponseStarted',
          details: {
            type: 'script',
            url: scriptUrl,
            statusCode: 200,
            ip: '127.0.0.1',
            fromCache: false,
            statusLine: 'HTTP/1.1 200 OK',
            initiator: getServerDomain(initiators.WEB_INITIATED, frontendHost)
          }
        },
        {
          label: 'onCompleted-2',
          event: 'onCompleted',
          details: {
            type: 'script',
            url: scriptUrl,
            statusCode: 200,
            ip: '127.0.0.1',
            fromCache: false,
            statusLine: 'HTTP/1.1 200 OK',
            initiator: getServerDomain(initiators.WEB_INITIATED, frontendHost)
          }
        },
      ],
      [[
        'onBeforeRequest-1', 'onBeforeSendHeaders-1', 'onSendHeaders-1',
        'onHeadersReceived-1', 'onResponseStarted-1', 'onCompleted-1',
        'onBeforeRequest-2', 'onBeforeSendHeaders-2', 'onSendHeaders-2',
        'onHeadersReceived-2', 'onResponseStarted-2', 'onCompleted-2'
      ]]);
}

// When the response is mocked via URLRequestMockHTTPJob, then the response
// differs from reality. Notably all header-related events are missing, the
// HTTP status line is different and the IP is missing.
// This difference is not a problem, since we are primarily interested in
// determining whether a request was observed or not.
function expectMockedTabNavigationEvents(url) {
  var scriptUrl = new URL(url);
  var frontendOrigin = scriptUrl.origin;
  scriptUrl.search = scriptUrl.hash = '';
  scriptUrl.pathname = scriptUrl.pathname.replace(/\.html$/, '.js');
  scriptUrl = scriptUrl.href;

  expect(
      [
        {
          label: 'onBeforeRequest-1',
          event: 'onBeforeRequest',
          details: {
            type: 'main_frame',
            url,
            frameUrl: url,
            initiator: getServerDomain(initiators.BROWSER_INITIATED)
          }
        },
        {
          label: 'onBeforeSendHeaders-1',
          event: 'onBeforeSendHeaders',
          details: {
            type: 'main_frame',
            url,
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
            requiredNetworkServiceState: "enabled"
          }
        },
        {
          label: 'onSendHeaders-1',
          event: 'onSendHeaders',
          details: {
            type: 'main_frame',
            url,
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
            requiredNetworkServiceState: "enabled"
          }
        },
        {
          label: 'onHeadersReceived-1',
          event: 'onHeadersReceived',
          details: {
            type: 'main_frame',
            url,
            statusCode: 200,
            statusLine: 'HTTP/1.0 200 OK',
            initiator: getServerDomain(initiators.BROWSER_INITIATED),
            requiredNetworkServiceState: "enabled"
          }
        },
        {
          label: 'onResponseStarted-1',
          event: 'onResponseStarted',
          details: {
            type: 'main_frame',
            url,
            statusCode: 200,
            fromCache: false,
            statusLine: 'HTTP/1.0 200 OK',
            initiator: getServerDomain(initiators.BROWSER_INITIATED)
          }
        },
        {
          label: 'onCompleted-1',
          event: 'onCompleted',
          details: {
            type: 'main_frame',
            url,
            statusCode: 200,
            fromCache: false,
            statusLine: 'HTTP/1.0 200 OK',
            initiator: getServerDomain(initiators.BROWSER_INITIATED)
          }
        },
        {
          label: 'onBeforeRequest-2',
          event: 'onBeforeRequest',
          details: {
            type: 'script',
            url: scriptUrl,
            frameUrl: url,
            // Cannot use getServerDomain(initiators.WEB_INITIATED) because it
            // always adds a port, while this request does not have any ports.
            initiator: frontendOrigin
          }
        },
        {
          label: 'onBeforeSendHeaders-2',
          event: 'onBeforeSendHeaders',
          details: {
            type: 'script',
            url: scriptUrl,
            initiator: frontendOrigin,
            requiredNetworkServiceState: "enabled"
          }
        },
        {
          label: 'onSendHeaders-2',
          event: 'onSendHeaders',
          details: {
            type: 'script',
            url: scriptUrl,
            initiator: frontendOrigin,
            requiredNetworkServiceState: "enabled"
          }
        },
        {
          label: 'onHeadersReceived-2',
          event: 'onHeadersReceived',
          details: {
            type: 'script',
            url: scriptUrl,
            statusCode: 200,
            statusLine: 'HTTP/1.0 200 OK',
            initiator: frontendOrigin,
            requiredNetworkServiceState: "enabled"
          }
        },
        {
          label: 'onResponseStarted-2',
          event: 'onResponseStarted',
          details: {
            type: 'script',
            url: scriptUrl,
            statusCode: 200,
            fromCache: false,
            statusLine: 'HTTP/1.0 200 OK',
            initiator: frontendOrigin
          }
        },
        {
          label: 'onCompleted-2',
          event: 'onCompleted',
          details: {
            type: 'script',
            url: scriptUrl,
            statusCode: 200,
            fromCache: false,
            statusLine: 'HTTP/1.0 200 OK',
            initiator: frontendOrigin
          }
        },
      ],
      [[
        'onBeforeRequest-1', 'onResponseStarted-1', 'onCompleted-1',
        'onBeforeRequest-2', 'onResponseStarted-2', 'onCompleted-2'
      ]]);
}

runTests([
  // Tests that chrome-devtools://devtools/custom/ is hidden from webRequest.
  function testDevToolsCustomFrontendRequest() {
    expectCompletionEvents();
    // DevToolsFrontendInWebRequestApuTest has set the kCustomDevtoolsFrontend
    // switch to the customfrontend/ subdirectory, so we do not include the path
    // name in the URL again.
    navigateAndWait(
        'chrome-devtools://devtools/custom/fakedevtools.html#' +
        getCompletionURL());
  },

  // Tests that the custom front-end URL is visible in non-DevTools requests.
  function testNonDevToolsCustomFrontendRequest() {
    // The URL that would be loaded by chrome-devtools://devtools/custom/...
    var customFrontendUrl = getServerURL(
        'devtoolsfrontend/fakedevtools.html', 'customfrontend.example.com');
    expectNormalTabNavigationEvents(customFrontendUrl);
    navigateAndWait(customFrontendUrl);
  },

  // Tests that chrome-devtools://devtools/remote/ is hidden from webRequest.
  function testDevToolsRemoteFrontendRequest() {
    expectCompletionEvents();
    navigateAndWait(
        'chrome-devtools://devtools/remote/devtoolsfrontend/fakedevtools.html' +
        '#' + getCompletionURL());
  },

  // Tests that the custom front-end URL is visible in non-DevTools requests.
  function testNonDevToolsRemoteFrontendRequest() {
    // The URL that would be loaded by chrome-devtools://devtools/remote/...
    var remoteFrontendUrl = 'https://chrome-devtools-frontend.appspot.com/' +
        'devtoolsfrontend/fakedevtools.html';
    expectMockedTabNavigationEvents(remoteFrontendUrl);
    navigateAndWait(remoteFrontendUrl);
  },
]);
