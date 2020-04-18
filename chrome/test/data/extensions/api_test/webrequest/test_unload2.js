// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

runTests([
  // Load a slow page in a tab and close it.
  function loadSlowTabAndClose() {
    const url = getSlowURL('slow-single-main-frame');

    expect([
      { label: 'onBeforeRequest',
        event: 'onBeforeRequest',
        details: {
          type: 'main_frame',
          url,
          frameUrl: url,
        }
      },
      { label: 'onBeforeSendHeaders',
        event: 'onBeforeSendHeaders',
        details: {
          type: 'main_frame',
          url,
        },
      },
      { label: 'onSendHeaders',
        event: 'onSendHeaders',
        details: {
          type: 'main_frame',
          url,
        },
      },
      { label: 'onErrorOccurred',
        event: 'onErrorOccurred',
        details: {
          type: 'main_frame',
          url,
          fromCache: false,
          error: 'net::ERR_ABORTED',
        },
      }],
      [['onBeforeRequest', 'onBeforeSendHeaders', 'onSendHeaders',
        'onErrorOccurred']]);

    var callbackDone = chrome.test.callbackAdded();

    waitUntilSendHeaders('main_frame', url, function() {
      // Cancels load and triggers onErrorOccurred.
      chrome.tabs.remove(tabId, callbackDone);
    }),
    chrome.tabs.update(tabId, {url});
  },
]);
