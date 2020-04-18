// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.getConfig(function(config) {
  chrome.test.runTests([
    function testCookieNotSet() {
      var xhr = new XMLHttpRequest();
      xhr.open(
          'GET',
          'http://localhost:' + config.testServer.port + '/echoheader?Cookie',
          true);
      xhr.onload = function() {
        // Cookies should not have been passed in the request.
        chrome.test.assertEq('', xhr.responseText);
        chrome.test.succeed();
      };
      xhr.onerror = function() {
        chrome.test.fail('Unexpected HTTP status ' + xhr.status);
      }
      xhr.send();
    }
  ]);
});
