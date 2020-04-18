// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
// Since all we want here is forwarding of certain commands, all can be done
// in the anonymous function's scope.

function wireUpWindow() {
  $('launch-button').addEventListener('click', function() {
    chrome.send('SetAsDefaultBrowser:LaunchSetDefaultBrowserFlow');
  });
}

window.addEventListener('DOMContentLoaded', wireUpWindow);
})();
