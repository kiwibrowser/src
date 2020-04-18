// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
'use strict';

function $(id) {
  return document.getElementById(id);
}

window.reloadWebview = function() {
    $('webview').reload();
    $('webview').focus();
};

window.addEventListener('DOMContentLoaded', function() {
  var webview = document.querySelector('#webview');
  webview.addEventListener('loadstop', function() {
    setTimeout(function() {
      chrome.test.sendMessage('WebViewTest.LAUNCHED');
    }, 0);
    if(document.hasFocus()) {
      chrome.test.sendMessage('WebViewTest.WEBVIEW_LOADED');
    } else {
      var wasFocused = function() {
        chrome.test.sendMessage('WebViewTest.WEBVIEW_LOADED');
        window.removeEventListener('focus', wasFocused);
      }
      window.addEventListener('focus', wasFocused);
    }
  });
  webview.focus();
  webview.src = 'focus_test.html';
});
