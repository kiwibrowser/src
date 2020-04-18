// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.tabs.getCurrent(function(tab) {
  runTestsForTab([
    // Opens a WebSocket connection, writes a message to it, and closes the
    // connection. WebRequest API should observe the entire handshake.
    function handshakeSucceeds() {
      var url = getWSTestURL(testWebSocketPort);
      expect(
        [  //events
          { label: 'onBeforeRequest',
            event: 'onBeforeRequest',
            details: {
              url: url,
              type: 'websocket',
              // TODO(pkalinnikov): Figure out why the frame URL is unknown.
              frameUrl: 'unknown frame URL',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
          },
          { label: 'onBeforeSendHeaders',
            event: 'onBeforeSendHeaders',
            details: {
              url: url,
              type: 'websocket',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
          },
          { label: 'onSendHeaders',
            event: 'onSendHeaders',
            details: {
              url: url,
              type: 'websocket',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
          },
          { label: 'onHeadersReceived',
            event: 'onHeadersReceived',
            details: {
              url: url,
              type: 'websocket',
              statusCode: 101,
              statusLine: 'HTTP/1.1 101 Switching Protocols',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
          },
          { label: 'onResponseStarted',
            event: 'onResponseStarted',
            details: {
              url: url,
              type: 'websocket',
              ip: '127.0.0.1',
              fromCache: false,
              statusCode: 101,
              statusLine: 'HTTP/1.1 101 Switching Protocols',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
          },
          { label: 'onCompleted',
            event: 'onCompleted',
            details: {
              url: url,
              type: 'websocket',
              ip: '127.0.0.1',
              fromCache: false,
              statusCode: 101,
              statusLine: 'HTTP/1.1 101 Switching Protocols',
              initiator: getDomain(initiators.WEB_INITIATED)
            }
          },
        ],
        [  // event order
          ['onBeforeRequest', 'onBeforeSendHeaders', 'onSendHeaders',
          'onHeadersReceived', 'onResponseStarted', 'onCompleted']
        ],
        {urls: ['<all_urls>']},  // filter
        ['blocking']  // extraInfoSpec
      );
      testWebSocketConnection(url, true /* expectedToConnect*/);
    },

    // Tries to open a WebSocket connection, with a blocking handler that
    // cancels the request. The connection will not be established.
    function handshakeRequestCancelled() {
      var url = getWSTestURL(testWebSocketPort);
      expect(
        [  // events
          { label: 'onBeforeRequest',
            event: 'onBeforeRequest',
            details: {
              url: url,
              type: 'websocket',
              frameUrl: 'unknown frame URL',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
            retval: {cancel: true}
          },
          // Cancelling is considered an error.
          { label: 'onErrorOccurred',
            event: 'onErrorOccurred',
            details: {
              url: url,
              type: 'websocket',
              fromCache: false,
              initiator: getDomain(initiators.WEB_INITIATED),
              error: 'net::ERR_BLOCKED_BY_CLIENT'
            }
          },
        ],
        [  // event order
          ['onBeforeRequest', 'onErrorOccurred']
        ],
        {urls: ['<all_urls>']},  // filter
        ['blocking']  // extraInfoSpec
      );
      testWebSocketConnection(url, false /* expectedToConnect*/);
    },

    // Opens a WebSocket connection, with a blocking handler that tries to
    // redirect the request. The redirect will be ignored.
    function redirectIsIgnoredAndHandshakeSucceeds() {
      var url = getWSTestURL(testWebSocketPort);
      var redirectedUrl1 = getWSTestURL(testWebSocketPort) + '?redirected1';
      var redirectedUrl2 = getWSTestURL(testWebSocketPort) + '?redirected2';
      expect(
        [  // events
          { label: 'onBeforeRequest',
            event: 'onBeforeRequest',
            details: {
              url: url,
              type: 'websocket',
              frameUrl: 'unknown frame URL',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
            retval: {redirectUrl: redirectedUrl1}
          },
          { label: 'onBeforeSendHeaders',
            event: 'onBeforeSendHeaders',
            details: {
              url: url,
              type: 'websocket',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
          },
          { label: 'onSendHeaders',
            event: 'onSendHeaders',
            details: {
              url: url,
              type: 'websocket',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
          },
          { label: 'onHeadersReceived',
            event: 'onHeadersReceived',
            details: {
              url: url,
              type: 'websocket',
              statusCode: 101,
              statusLine: 'HTTP/1.1 101 Switching Protocols',
              initiator: getDomain(initiators.WEB_INITIATED)
            },
            retval: {redirectUrl: redirectedUrl2}
          },
          { label: 'onResponseStarted',
            event: 'onResponseStarted',
            details: {
              url: url,
              type: 'websocket',
              ip: '127.0.0.1',
              fromCache: false,
              initiator: getDomain(initiators.WEB_INITIATED),
              statusCode: 101,
              statusLine: 'HTTP/1.1 101 Switching Protocols',
            },
          },
          { label: 'onCompleted',
            event: 'onCompleted',
            details: {
              url: url,
              type: 'websocket',
              ip: '127.0.0.1',
              fromCache: false,
              initiator: getDomain(initiators.WEB_INITIATED),
              statusCode: 101,
              statusLine: 'HTTP/1.1 101 Switching Protocols',
            }
          },
        ],
        [  // event order
          ['onBeforeRequest', 'onBeforeSendHeaders', 'onHeadersReceived',
          'onResponseStarted', 'onCompleted']
        ],
        {urls: ['<all_urls>']},  // filter
        ['blocking']  // extraInfoSpec
      );
      testWebSocketConnection(url, true /* expectedToConnect*/);
    },
  ], tab);
});
