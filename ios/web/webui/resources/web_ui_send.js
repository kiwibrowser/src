// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.require('__crWeb.webUIBase');

goog.provide('__crWeb.webUISend');

window['chrome']['send'] = function(message, args) {
  __gCrWeb.message.invokeOnHost({'command': 'chrome.send',
                                 'message': '' + message,
                                 'arguments': args || []});
};
