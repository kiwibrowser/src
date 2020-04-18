// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  function getVolumeList() {
    chrome.fileSystem.getVolumeList(
        chrome.test.callbackPass(function(volumeList) {
          // Drive is not available in a real kiosk session. Hhowever, this test
          // runs in a normal session (with a user marked as kiosk user) since
          // executing a real real kiosk session is tests is very complicated.
          // Whether Drive is available in the real kiosk session is tested
          // separetely in: chrome/browser/chromeos/login/kiosk_browsertest.cc.
          chrome.test.assertEq(4, volumeList.length);
          chrome.test.assertTrue(/^downloads:.*/.test(volumeList[0].volumeId));
          chrome.test.assertTrue(volumeList[0].writable);
          chrome.test.assertEq('drive:drive-user', volumeList[1].volumeId);
          chrome.test.assertTrue(volumeList[1].writable);

          chrome.test.assertEq('testing:read-only', volumeList[2].volumeId);
          chrome.test.assertFalse(volumeList[2].writable);
          chrome.test.assertEq('testing:writable', volumeList[3].volumeId);
          chrome.test.assertTrue(volumeList[3].writable);
        }));
  }
]);
