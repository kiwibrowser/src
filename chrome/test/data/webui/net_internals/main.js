// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Include test fixture.
GEN_INCLUDE(['net_internals_test.js']);

// Anonymous namespace
(function() {

/**
 * Checks visibility of tab handles against expectations, and navigates to all
 * tabs with visible handles, validating visibility of all other tabs as it
 * goes.
 */
TEST_F('NetInternalsTest', 'netInternalsTourTabs', function() {
  // Prevent sending any events to the browser as we flip through tabs, since
  // this test is just intended to make sure everything's created and hooked
  // up properly Javascript side.
  g_browser.disable();

  // Expected visibility state of each tab.
  var tabVisibilityState = {
    capture: true,
    import: true,
    proxy: true,
    events: true,
    timeline: true,
    dns: true,
    sockets: true,
    http2: true,
    quic: true,
    reporting: true,
    'alt-svc': true,
    httpCache: true,
    modules: true,
    hsts: true,
    prerender: true,
    bandwidth: true,
    chromeos: cr.isChromeOS
  };

  NetInternalsTest.checkTabLinkVisibility(tabVisibilityState, true);

  testDone();
});

})();  // Anonymous namespace
