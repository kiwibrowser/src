// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([

  function nocompileFunc() {
    chrome.test.assertEq("function", typeof(chrome.idltest.nocompileFunc));
    chrome.test.succeed();
  }

]);
