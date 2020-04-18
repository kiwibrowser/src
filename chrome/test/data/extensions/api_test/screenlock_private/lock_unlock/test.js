// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var stage = 0;

function step() {
  chrome.screenlockPrivate.getLocked(function gotLocked(locked) {
    if (stage === 0) {
      if (locked) {
        chrome.test.fail('locked at stage ' + stage);
      } else {
        chrome.screenlockPrivate.setLocked(true);
        ++stage;
        return true;
      }
    } else if (stage === 1) {
      if (!locked) {
        chrome.test.fail('unlocked at stage ' + stage);
      } else {
        chrome.screenlockPrivate.setLocked(false);
        ++stage;
        return true;
      }
    } else if (stage === 2) {
      if (locked) {
        chrome.test.fail('locked at stage ' + stage);
      } else {
        chrome.test.succeed();
      }
    }
  });
};

chrome.screenlockPrivate.onChanged.addListener(function(locked) {
  if (locked != (1 == (stage % 2))) {
    chrome.test.fail((locked ? '' : 'un') + 'locked at stage ' + stage +
                     ' onChanged');
  } else {
    return step();
  }
});

chrome.test.runTests([
    function testLockUnlock() {
      step();
    }
]);
