// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

var resultQueue = new ResultQueue();

// Sends data back to the test. This must be in response to an earlier
// request, but it's ok to respond asynchronously. The request blocks until
// the response is sent.
function sendResultToTest(result) {
  console.log('sendResultToTest: ' + result);
  if (window.domAutomationController) {
    domAutomationController.send('' + result);
  }
}

function sendErrorToTest(error) {
  sendResultToTest(error.name + ' - ' + error.message);
}

function registerServiceWorker() {
  navigator.serviceWorker.register('service_worker.js', {scope: './'})
    .then(function() {
      return navigator.serviceWorker.ready;
    })
    .then(function(swRegistration) {
      sendResultToTest('ok - service worker registered');
    })
    .catch(sendErrorToTest);
}

function register(tag) {
  navigator.serviceWorker.ready
    .then(function(swRegistration) {
      return swRegistration.sync.register(tag);
    })
    .then(function() {
      sendResultToTest('ok - ' + tag + ' registered');
    })
    .catch(sendErrorToTest);
}

function registerFromServiceWorker(tag) {
  navigator.serviceWorker.ready
    .then(function(swRegistration) {
      swRegistration.active.postMessage({action: 'register', tag: tag});
      sendResultToTest('ok - ' + tag + ' register sent to SW');
    })
    .catch(sendErrorToTest);
}

function hasTag(tag) {
  navigator.serviceWorker.ready
    .then(function(swRegistration) {
      return swRegistration.sync.getTags();
    })
    .then(function(tags) {
      if (tags.indexOf(tag) >= 0) {
        sendResultToTest('ok - ' + tag + ' found');
      } else {
        sendResultToTest('error - ' + tag + ' not found');
        return;
      }
    })
    .catch(sendErrorToTest);
}

function hasTagFromServiceWorker(tag) {
  navigator.serviceWorker.ready
    .then(function(swRegistration) {
      swRegistration.active.postMessage(
          {action: 'hasTag', tag: tag});
      sendResultToTest('ok - hasTag sent to SW');
    })
    .catch(sendErrorToTest);
}

function getTags() {
  navigator.serviceWorker.ready
    .then(function(swRegistration) {
      return swRegistration.sync.getTags();
    })
    .then(function(tags) {
      sendResultToTest('ok - ' + tags.toString());
    })
    .catch(sendErrorToTest);
}

function getTagsFromServiceWorker() {
  navigator.serviceWorker.ready
    .then(function(swRegistration) {
      swRegistration.active.postMessage({action: 'getTags'});
      sendResultToTest('ok - getTags sent to SW');
    })
    .catch(sendErrorToTest);
}

function completeDelayedSyncEvent() {
  navigator.serviceWorker.ready
    .then(function(swRegistration) {
      swRegistration.active.postMessage({
          action: 'completeDelayedSyncEvent'
        });
      sendResultToTest('ok - delay completing');
    })
    .catch(sendErrorToTest);
}

function rejectDelayedSyncEvent() {
  navigator.serviceWorker.ready
    .then(function(swRegistration) {
      swRegistration.active.postMessage({action: 'rejectDelayedSyncEvent'});
      sendResultToTest('ok - delay rejecting');
    })
    .catch(sendErrorToTest);
}

function createFrame(url) {
  return new Promise(function(resolve) {
    var frame = document.createElement('iframe');
    frame.src = url;
    frame.onload = function() { resolve(frame); };
    document.body.appendChild(frame);
  });
}

function registerFromLocalFrame(frame_url) {
  var frameWindow;
  return createFrame(frame_url)
    .then(function(frame) {
      frameWindow = frame.contentWindow;
      return frameWindow.navigator.serviceWorker.register('service_worker.js');
    })
    .then(function() {
      return frameWindow.navigator.serviceWorker.ready;
    })
    .then(function(frame_registration) {
      return frame_registration.sync.register('foo');
    })
    .then(function() {
      sendResultToTest('ok - iframe registered sync');
    })
    .catch(sendErrorToTest);
}

function receiveMessage() {
  return new Promise(function(resolve) {
    window.addEventListener('message', function(message) {
      resolve(message.data);
    });
  });
}

function registerFromCrossOriginFrame(cross_frame_url) {
  return createFrame(cross_frame_url)
    .then(function(frame) {
      return receiveMessage();
    })
    .then(function(message) {
      console.log(message);
      if (message !== 'registration failed') {
        sendResultToTest('failed - ' + message);
        return;
      }
      sendResultToTest('ok - frame failed to register sync');
    });
}

// Queue storing asynchronous results received from the Service Worker. Results
// are sent to the test when requested.
function ResultQueue() {
  // Invariant: this.queue.length == 0 || this.pendingGets == 0
  this.queue = [];
  this.pendingGets = 0;
}

// Adds a data item to the queue. Will be sent to the test if there are
// pendingGets.
ResultQueue.prototype.push = function(data) {
  if (this.pendingGets) {
    this.pendingGets--;
    sendResultToTest(data);
  } else {
    this.queue.unshift(data);
  }
};

// Called by native. Sends the next data item to the test if it is available.
// Otherwise increments pendingGets so it will be delivered when received.
ResultQueue.prototype.pop = function() {
  if (this.queue.length) {
    sendResultToTest(this.queue.pop());
  } else {
    this.pendingGets++;
  }
};

// Called by native. Immediately sends the next data item to the test if it is
// available, otherwise sends null.
ResultQueue.prototype.popImmediately = function() {
  sendResultToTest(this.queue.length ? this.queue.pop() : null);
};

navigator.serviceWorker.addEventListener('message', function(event) {
  var message = event.data;
  if (message.type == 'sync' || message.type === 'register')
    resultQueue.push(message.data);
}, false);
