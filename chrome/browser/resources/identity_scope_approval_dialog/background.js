// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Displays a webview based authorization dialog.
 * @param {string} key A unique identifier that the caller can use to locate
 *     the dialog window.
 * @param {string} url A URL that will be loaded in the webview.
 * @param {string} mode 'interactive' or 'silent'. The window will be displayed
 *     if the mode is 'interactive'.
 */
function showAuthDialog(key, url, mode) {
  var options =
      {frame: 'none', id: key, minWidth: 1024, minHeight: 768, hidden: true};
  chrome.app.window.create(
      'scope_approval_dialog.html', options, function(win) {
        win.contentWindow.addEventListener('load', function(event) {
          var windowParam;
          if (mode == 'interactive')
            windowParam = win;
          win.contentWindow.loadAuthUrlAndShowWindow(url, windowParam);
        });
      });
}

chrome.identityPrivate.onWebFlowRequest.addListener(showAuthDialog);
