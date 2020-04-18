// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// We should not be able to read the "num" variable which was defined in a.js
// from the "isolated world 1" extension.
chrome.runtime.connect().postMessage(typeof num == "undefined");
