// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.cast.channel.open({
  ipAddress: '192.168.1.1',
  port: 8009,
  auth: 'ssl_verified',
  livenessTimeout: 5000,
  pingInterval: 1000
}, function(channel) {
  assertOpenChannel(channel);
  console.log(JSON.stringify(channel));
  chrome.test.assertEq(channel.keepAlive, true);
  chrome.cast.channel.onError.addListener(
      function(channel, error) {
        chrome.test.assertEq(channel.keepAlive, true);
        if (channel.readyState == 'closed' &&
            error.errorState == 'ping_timeout') {
          chrome.cast.channel.close(channel, () => {
            chrome.test.sendMessage('timeout_ssl_verified');
          });
        }
      });
  chrome.test.notifyPass();
  chrome.test.sendMessage('channel_opened_ssl_verified');
});
