// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function onModuleLoaded() {
  chrome.test.notifyPass();
}

window.onload = function() {
  var nacl_module = document.getElementById('nacl_module');
  nacl_module.addEventListener('load', onModuleLoaded, false);
};
