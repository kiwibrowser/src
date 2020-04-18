// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Register this extension as the network handler for wifi1.
chrome.networking.config.setNetworkFilter([{Type: 'WiFi', SSID: 'wifi1'}],
                                          function() {
                                            chrome.test.sendMessage('done');
                                          });
