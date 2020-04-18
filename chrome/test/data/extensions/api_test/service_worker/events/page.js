// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var workerPromise = new Promise(function(resolve, reject) {
  navigator.serviceWorker.register('sw.js').then(function() {
    return navigator.serviceWorker.ready;
  }).then(function(registration) {
    var sw = registration.active;
    var channel = new MessageChannel();
    channel.port1.onmessage = function(e) {
      var data = e.data;
      if (data == 'listener-added') {
        var url = chrome.extension.getURL('on_updated.html');
        chrome.tabs.create({url: url});
      } else if (data == 'chrome.tabs.onUpdated callback') {
        resolve(data);
      } else {
        reject(data);
      }
    };
    sw.postMessage('addOnUpdatedListener', [channel.port2]);
  }).catch(function(err) {
    reject(err);
  });
});

window.runEventTest = function() {
  workerPromise.then(function(message) {
    window.domAutomationController.send(message);
  }).catch(function(err) {
    window.domAutomationController.send('FAILURE');
  });
};
