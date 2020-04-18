// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Adds filter script and css to all existing tabs.
 *
 * TODO(wnwen): Verify content scripts are not being injected multiple times.
 */
function injectContentScripts() {
  chrome.windows.getAll({'populate': true}, function(windows) {
    for (var i = 0; i < windows.length; i++) {
      var tabs = windows[i].tabs;
      for (var j = 0; j < tabs.length; j++) {
        var url = tabs[j].url;
        if (isDisallowedUrl(url)) {
          continue;
        }
        chrome.tabs.executeScript(
            tabs[j].id,
            {file: 'src/common.js'});
        chrome.tabs.executeScript(
            tabs[j].id,
            {file: 'src/cvd.js'});
      }
    }
  });
}

/**
 * Updates all existing tabs with config values.
 */
function updateTabs() {
  chrome.windows.getAll({'populate': true}, function(windows) {
    for (var i = 0; i < windows.length; i++) {
      var tabs = windows[i].tabs;
      for (var j = 0; j < tabs.length; j++) {
        var url = tabs[j].url;
        if (isDisallowedUrl(url)) {
          continue;
        }
        var msg = {
          'delta': getSiteDelta(siteFromUrl(url)),
          'severity': getDefaultSeverity(),
          'type': getDefaultType(),
          'simulate': getDefaultSimulate(),
          'enable': getDefaultEnable()
        };
        debugPrint('updateTabs: sending ' + JSON.stringify(msg) + ' to ' +
            siteFromUrl(url));
        chrome.tabs.sendRequest(tabs[j].id, msg);
      }
    }
  });
}

/**
 * Initial extension loading.
 */
(function initialize() {
  injectContentScripts();
  updateTabs();

  chrome.extension.onRequest.addListener(
      function(request, sender, sendResponse) {
        if (request['init']) {
          var delta = getDefaultDelta();
          if (sender.tab) {
            delta = getSiteDelta(siteFromUrl(sender.tab.url));
          }

          var msg = {
            'delta': delta,
            'severity': getDefaultSeverity(),
            'type': getDefaultType(),
            'simulate': getDefaultSimulate(),
            'enable': getDefaultEnable()
          };
          sendResponse(msg);
        }
      });

  //TODO(mustaq): Handle uninstall

  document.addEventListener('storage', function(evt) {
    updateTabs();
  }, false);
})();
