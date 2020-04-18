// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var onMessageReply = function(message) {
  var enabled = (message == 'start enabled');
  var id = chrome.contextMenus.create({'title': 'Extension Item 1',
                                       'enabled': enabled}, function() {
    chrome.test.sendMessage('create', function(message) {
      chrome.contextMenus.update(id, {'enabled': !enabled}, function() {
        chrome.test.sendMessage('update');
      });
    });
  });
};

chrome.test.sendMessage('begin', onMessageReply);
