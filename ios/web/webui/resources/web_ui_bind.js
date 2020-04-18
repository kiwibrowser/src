// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('__crWeb.webUIBind');

// Chrome defines bind on all functions, so this is expected to exist by
// webui's scripts.
Function.prototype.bind = function(context) {
  // Reference to the Function instance.
  var self = this;
  // Reference to the current arguments.
  var curriedArguments = [];
  for (var i = 1; i < arguments.length; i++)
    curriedArguments.push(arguments[i]);
  return function() {
    var finalArguments = [];
    for (var i = 0; i < curriedArguments.length; i++)
      finalArguments.push(curriedArguments[i]);
    for (var i = 0; i < arguments.length; i++)
      finalArguments.push(arguments[i]);
    return self.apply(context, finalArguments);
  }
};
