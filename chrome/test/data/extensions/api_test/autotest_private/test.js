// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  // logout/restart/shutdown don't do anything as we don't want to kill the
  // browser with these tests.
  function logout() {
    chrome.autotestPrivate.logout();
    chrome.test.succeed();
  },
  function restart() {
    chrome.autotestPrivate.restart();
    chrome.test.succeed();
  },
  function shutdown() {
    chrome.autotestPrivate.shutdown(true);
    chrome.test.succeed();
  },
  function lockScreen() {
    chrome.autotestPrivate.lockScreen();
    chrome.test.succeed();
  },
  function simulateAsanMemoryBug() {
    chrome.autotestPrivate.simulateAsanMemoryBug();
    chrome.test.succeed();
  },
  function loginStatus() {
    chrome.autotestPrivate.loginStatus(
        chrome.test.callbackPass(function(status) {
          chrome.test.assertEq(typeof(status), 'object');
          chrome.test.assertTrue(status.hasOwnProperty("isLoggedIn"));
          chrome.test.assertTrue(status.hasOwnProperty("isOwner"));
          chrome.test.assertTrue(status.hasOwnProperty("isScreenLocked"));
          chrome.test.assertTrue(status.hasOwnProperty("isRegularUser"));
          chrome.test.assertTrue(status.hasOwnProperty("isGuest"));
          chrome.test.assertTrue(status.hasOwnProperty("isKiosk"));
          chrome.test.assertTrue(status.hasOwnProperty("email"));
          chrome.test.assertTrue(status.hasOwnProperty("displayEmail"));
          chrome.test.assertTrue(status.hasOwnProperty("userImage"));
        }));
  },
  function getExtensionsInfo() {
    chrome.autotestPrivate.getExtensionsInfo(
        chrome.test.callbackPass(function(extInfo) {
          chrome.test.assertEq(typeof(extInfo), 'object');
          chrome.test.assertTrue(extInfo.hasOwnProperty('extensions'));
          chrome.test.assertTrue(extInfo.extensions.constructor === Array);
          for (var i = 0; i < extInfo.extensions.length; ++i) {
            var extension = extInfo.extensions[i];
            chrome.test.assertTrue(extension.hasOwnProperty('id'));
            chrome.test.assertTrue(extension.hasOwnProperty('version'));
            chrome.test.assertTrue(extension.hasOwnProperty('name'));
            chrome.test.assertTrue(extension.hasOwnProperty('publicKey'));
            chrome.test.assertTrue(extension.hasOwnProperty('description'));
            chrome.test.assertTrue(extension.hasOwnProperty('backgroundUrl'));
            chrome.test.assertTrue(extension.hasOwnProperty(
                'hostPermissions'));
            chrome.test.assertTrue(
                extension.hostPermissions.constructor === Array);
            chrome.test.assertTrue(extension.hasOwnProperty(
                'effectiveHostPermissions'));
            chrome.test.assertTrue(
                extension.effectiveHostPermissions.constructor === Array);
            chrome.test.assertTrue(extension.hasOwnProperty(
                'apiPermissions'));
            chrome.test.assertTrue(
                extension.apiPermissions.constructor === Array);
            chrome.test.assertTrue(extension.hasOwnProperty('isComponent'));
            chrome.test.assertTrue(extension.hasOwnProperty('isInternal'));
            chrome.test.assertTrue(extension.hasOwnProperty(
                'isUserInstalled'));
            chrome.test.assertTrue(extension.hasOwnProperty('isEnabled'));
            chrome.test.assertTrue(extension.hasOwnProperty(
                'allowedInIncognito'));
            chrome.test.assertTrue(extension.hasOwnProperty('hasPageAction'));
          }
        }));
  },
  function setTouchpadSensitivity() {
    chrome.autotestPrivate.setTouchpadSensitivity(3);
    chrome.test.succeed();
  },
  function setTapToClick() {
    chrome.autotestPrivate.setTapToClick(true);
    chrome.test.succeed();
  },
  function setThreeFingerClick() {
    chrome.autotestPrivate.setThreeFingerClick(true);
    chrome.test.succeed();
  },
  function setTapDragging() {
    chrome.autotestPrivate.setTapDragging(false);
    chrome.test.succeed();
  },
  function setNaturalScroll() {
    chrome.autotestPrivate.setNaturalScroll(true);
    chrome.test.succeed();
  },
  function setMouseSensitivity() {
    chrome.autotestPrivate.setMouseSensitivity(3);
    chrome.test.succeed();
  },
  function setPrimaryButtonRight() {
    chrome.autotestPrivate.setPrimaryButtonRight(false);
    chrome.test.succeed();
  },
  function setMouseReverseScroll() {
    chrome.autotestPrivate.setMouseReverseScroll(true);
    chrome.test.succeed();
  },
  function getVisibleNotifications() {
    chrome.autotestPrivate.getVisibleNotifications(function(){});
    chrome.test.succeed();
  },
  function getPlayStoreState() {
    chrome.autotestPrivate.getPlayStoreState(function(state) {
      // By default ARC is not available. Field allowed must be set to false;
      // managed and enabled should be underfined.
      chrome.test.assertFalse(state.allowed);
      chrome.test.assertEq(undefined, state.enabled);
      chrome.test.assertEq(undefined, state.managed);
      chrome.test.succeed();
    });
  },
  function setPlayStoreEnabled() {
    chrome.autotestPrivate.setPlayStoreEnabled(false, function() {
      // By default ARC is not available.
      chrome.test.assertTrue(chrome.runtime.lastError != undefined);
      chrome.test.succeed();
    });
  },
  function getPrinterList() {
    chrome.autotestPrivate.getPrinterList(function(){
      chrome.test.succeed();
    });
  }
]);
