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
  chrome.test.assertLastError('Channel socket error = 3');
  chrome.test.succeed();
};

var onSend = function(channel) {
  chrome.test.assertLastError('Channel error = 1');
  chrome.cast.channel.close(channel, onClose);
};

var onOpen = function(channel) {
  chrome.test.assertLastError('Channel socket error = 3');
  assertClosedChannelWithError(channel, 'connect_error');
  chrome.cast.channel.send(channel, message, onSend);
};

chrome.cast.channel.open({
  ipAddress: '192.168.1.1',
  port: 8009,
  auth: 'ssl_verified'}, onOpen);
