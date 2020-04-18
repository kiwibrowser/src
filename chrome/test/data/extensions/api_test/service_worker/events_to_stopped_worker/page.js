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

window.runServiceWorker = function() {
  readyPromise.then(function(message) {
    window.domAutomationController.send(message);
  }).catch(function(err) {
    window.domAutomationController.send('FAILURE');
  });
};

window.createTabThenUpdate = function() {
  navigator.serviceWorker.onmessage = function(e) {
    // e.data -> 'chrome.tabs.onUpdated callback'.
    window.domAutomationController.send(e.data);
  };
  var url = chrome.extension.getURL('on_updated.html');
  chrome.tabs.create({url: url});
};
