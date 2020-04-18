// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


// Notify the extension needs to show the page action icon.
chrome.extension.sendRequest({msg: "feedIcon"});
