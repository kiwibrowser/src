// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
// The Mozilla DHTML performance tests need to explicitly call a function to
// trigger the next page visit, rather than directly using the onload handler.
// To meet needs of the DHTML performance tests without forking this head.js
// file, use a variable |__install_onload_handler| to indicate whether the
// |__onload| handler should be added to onload event listener.
// Install |__onload| by default if there is no pre-configuration.
if (typeof(__install_onload_handler) == 'undefined')
  var __install_onload_handler = true;

// This is the timeout used in setTimeout inside the DHTML tests.  Chrome has
// a much more accurate timer resolution than other browsers do.  This results
// in Chrome running these tests much faster than other browsers.  In order to
// compare Chrome with other browsers on DHTML performance alone, set this
// value to ~15.
var __test_timeout = 0;

function __set_cookie(name, value) {
  document.cookie = name + "=" + value + "; path=/";
}

function __onbeforeunload() {
  // Call GC twice to cleanup JS heap before starting a new test.
  if (window.gc) {
    window.gc();
    window.gc();
  }
}

// The function |__onload| is used by the DHTML tests.
window.__onload = function() {
  if (!__install_onload_handler && !performance.timing.loadEventEnd)
    return;

  var unused = document.body.offsetHeight;  // force layout

  window.__pc_load_time = window.performance.now();
};

// The function |__eval_later| now is only used by the DHTML tests.
window.__eval_later = function(expression) {
  setTimeout(expression, __test_timeout);
};

if (window.parent == window) {  // Ignore subframes.
  window.__pc_load_time = null;
  addEventListener("load", __onload);
  addEventListener("beforeunload", __onbeforeunload);
}
})();