// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var serviceWorkerPromise = new Promise(function(resolve, reject) {
  navigator.serviceWorker.register('sw.js').then(function() {
    return navigator.serviceWorker.ready;
  }).then(function(registration) {
    var sw = registration.active;
    var channel = new MessageChannel();
    channel.port1.onmessage = function(e) {
      if (e.data == 'chrome.tabs.create callback') {
        resolve(e.data);
      } else {
        reject(e.data);
      }
    };
    sw.postMessage('createTab', [channel.port2]);
  }).catch(function(err) {
    reject(err);
  });
});

window.runServiceWorker = function() {
  serviceWorkerPromise.then(function(message) {
    window.domAutomationController.send(message);
  }).catch(function(err) {
    window.domAutomationController.send('FAILURE');
  });
};
