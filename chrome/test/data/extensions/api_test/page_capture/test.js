// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// API test for chrome.extension.pageCapture.
// browser_tests.exe --gtest_filter=ExtensionPageCaptureApiTest.*

const assertEq = chrome.test.assertEq;
const assertTrue = chrome.test.assertTrue;

var testUrl = 'http://www.a.com:PORT' +
    '/extensions/api_test/page_capture/google.html';

function waitForCurrentTabLoaded(callback) {
  chrome.tabs.getSelected(null, function(tab) {
    if (tab.status == "complete" && tab.url == testUrl) {
      callback();
      return;
    }
    window.setTimeout(function() { waitForCurrentTabLoaded(callback); }, 100);
  });
}

chrome.test.getConfig(function(config) {
  testUrl = testUrl.replace(/PORT/, config.testServer.port);

  chrome.test.runTests([
    function saveAsMHTML() {
      chrome.tabs.getSelected(null, function(tab) {
        chrome.tabs.update(null, { "url": testUrl });
        waitForCurrentTabLoaded(function() {
          chrome.pageCapture.saveAsMHTML({ "tabId": tab.id },
              function(data) {
            if (config.customArg == "REQUEST_DENIED") {
              chrome.test.assertLastError("User denied request.");
              chrome.test.notifyPass();
              return;
            }
            assertEq(undefined, chrome.runtime.lastError);
            assertTrue(data != null);
            // It should contain few KBs of data.
            assertTrue(data.size > 100);
            // Let's make sure it contains some well known strings.
            var reader = new FileReader();
            reader.onload = function(e) {
              var text = e.target.result;
              assertTrue(text.indexOf(testUrl) != -1);
              assertTrue(text.indexOf("logo.png") != -1);
              // Run the GC so the blob is deleted.
              window.setTimeout(function() { window.gc(); });
              window.setTimeout(function() { chrome.test.notifyPass(); }, 0);
            };
            reader.readAsText(data);
          });
        });
      });
    }
  ]);
});

