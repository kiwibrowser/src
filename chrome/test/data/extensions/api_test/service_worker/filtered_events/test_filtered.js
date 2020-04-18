// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var workerPromise = new Promise(function(resolve, reject) {
  navigator.serviceWorker.register('sw.js').then(function() {
    return navigator.serviceWorker.ready;
  }).then(function(registration) {
    var sw = registration.active;
    sw.postMessage('testServiceWorkerFilteredEvents');
  }).catch(function(err) {
    reject(err);
  });
});


function testServiceWorkerFilteredEvents() {
  // The worker calls chrome.test.succeed() if the test passes.
  workerPromise.catch(function(err) {
    chrome.test.fail();
  });
};

chrome.test.runTests([testServiceWorkerFilteredEvents]);
