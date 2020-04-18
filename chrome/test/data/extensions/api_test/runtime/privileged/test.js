// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var assertEq = chrome.test.assertEq;
var assertTrue = chrome.test.assertTrue;
var fail = chrome.test.fail;
var succeed = chrome.test.succeed;

function checkIsDefined(prop) {
  if (!chrome.runtime) {
    fail('chrome.runtime is not defined');
    return false;
  }
  if (!chrome.runtime[prop]) {
    fail('chrome.runtime.' + prop + ' is not undefined');
    return false;
  }
  return true;
}

chrome.test.runTests([

  function testGetURL() {
    if (!checkIsDefined('getURL'))
      return;
    var url = chrome.runtime.getURL('_generated_background_page.html');
    assertEq(url, window.location.href);
    succeed();
  },

  function testGetManifest() {
    if (!checkIsDefined('getManifest'))
      return;
    var manifest = chrome.runtime.getManifest();
    if (!manifest || !manifest.background || !manifest.background.scripts) {
      fail();
      return;
    }
    assertEq(manifest.name, 'chrome.runtime API Test');
    assertEq(manifest.version, '1');
    assertEq(manifest.manifest_version, 2);
    assertEq(manifest.background.scripts, ['test.js']);
    succeed();
  },

  function testID() {
    if (!checkIsDefined('id'))
      return;
    // We *could* get the browser to tell the test what the extension ID is,
    // and compare against that. It's a pain. Testing for a non-empty ID should
    // be good enough.
    assertTrue(chrome.runtime.id != '', 'chrome.runtime.id is empty-string.');
    succeed();
  }

]);
