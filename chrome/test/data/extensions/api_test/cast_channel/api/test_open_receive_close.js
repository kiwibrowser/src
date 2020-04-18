// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var message = {
  'namespace_': 'foo',
  'sourceId': 'src',
  'destinationId': 'dest',
  'data': 'some-string'
};

var onClose = function(channel) {
  assertClosedChannel(channel);
  chrome.test.succeed();
};

var numMessages = 0;

var onMessage = function(channel, message) {
  assertOpenChannel(channel);
  ++numMessages;
  if (numMessages == 2) {  // Expect a couple of messages.
    chrome.cast.channel.close(channel, onClose);
  }
};

var onOpen = function(channel) {
  assertOpenChannel(channel);
  chrome.cast.channel.onMessage.addListener(onMessage);
  chrome.test.notifyPass();
};

chrome.cast.channel.open({
  ipAddress: '192.168.1.1',
  port: 8009,
  auth: 'ssl_verified'}, onOpen);
