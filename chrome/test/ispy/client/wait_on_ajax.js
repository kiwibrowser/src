// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var target = document.body;
var callback = arguments[arguments.length - 1]

var timeout_id = setTimeout(function() {
  callback()
}, 5000);

var observer = new MutationObserver(function(mutations) {
  clearTimeout(timeout_id);
  timeout_id = setTimeout(function() {
    callback();
  }, 5000);
}).observe(target, {attributes: true, childList: true,
  characterData: true, subtree: true});
