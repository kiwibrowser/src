// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var readyPromise = new Promise(function(resolve, reject) {
  navigator.serviceWorker.register('sw.js').then(function() {
    return navigator.serviceWorker.ready;
  }).then(function(registration) {
    resolve('ready');
  }).catch(function(err) {
    reject(err);
  });
});

window.runServiceWorkerAsync = function() {
  chrome.test.log('runServiceWorkerAsync');
  readyPromise.then(function(message) {
    chrome.test.sendMessage('listener-added');
  }).catch(function(err) {
    chrome.test.sendMessage('FAILURE');
  });
};
