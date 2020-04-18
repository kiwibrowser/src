// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function check_overflow_check(value) {
  try {
    chrome.windows.create({ "left": value }, function() { });
  } catch (e) {
    var jsBindingsError = 'Value must fit in a 32-bit signed integer.';
    var nativeBindingsError = 'Invalid type: expected integer, found number.';
    chrome.test.assertTrue(
        e.message.indexOf(jsBindingsError) != -1 ||
        e.message.indexOf(nativeBindingsError) != -1,
        e.message);
    chrome.test.succeed();
    return;
  }
}
chrome.test.runTests([
  function overflow2To31() { check_overflow_check(0x80000000); },
  function overflowMinus2To31Minus1() { check_overflow_check(-0x80000001); },
  function overflow2To32() { check_overflow_check(0x100000000); },
]);
