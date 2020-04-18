// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var webview;

/**
 * Points the webview to the starting URL of a scope authorization
 * flow, and unhides the dialog once the page has loaded.
 * @param {string} url The url of the authorization entry point.
 * @param {Object} win The dialog window that contains this page. Can
 *     be left undefined if the caller does not want to display the
 *     window.
 */
function loadAuthUrlAndShowWindow(url, win) {
  // Send popups from the webview to a normal browser window.
  webview.addEventListener('newwindow', function(e) {
    e.window.discard();
    window.open(e.targetUrl);
  });

  // Request a customized view from GAIA.
  webview.request.onBeforeSendHeaders.addListener(
      function(details) {
        headers = details.requestHeaders || [];
        headers.push({'name': 'X-Browser-View', 'value': 'embedded'});
        return {requestHeaders: headers};
      },
      {
        urls: ['https://accounts.google.com/*'],
      },
      ['blocking', 'requestHeaders']);

  if (!url.toLowerCase().startsWith('https://accounts.google.com/'))
    document.querySelector('.titlebar').classList.add('titlebar-border');

  webview.src = url;
  if (win) {
    webview.addEventListener('loadstop', function() {
      win.show();
    });
  }
}

document.addEventListener('DOMContentLoaded', function() {
  webview = document.querySelector('webview');

  document.querySelector('.titlebar-close-button').onclick = function() {
    window.close();
  };

  chrome.resourcesPrivate.getStrings('identity', function(strings) {
    document.title = strings['window-title'];
  });
});
