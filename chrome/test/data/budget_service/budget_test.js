// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

// The ResultQueue is a mechanism for passing messages back to the test
// framework.
var resultQueue = new ResultQueue();

// Waits for the given ServiceWorkerRegistration to become ready.
// Shim for https://github.com/w3c/ServiceWorker/issues/770.
function swRegistrationReady(reg) {
  return new Promise((resolve, reject) => {
    if (reg.active) {
      resolve();
      return;
    }

    if (!reg.installing && !reg.waiting) {
      reject(Error('Install failed'));
      return;
    }

    (reg.installing || reg.waiting).addEventListener('statechange', function() {
      if (this.state == 'redundant') {
        reject(Error('Install failed'));
      } else if (this.state == 'activated') {
        resolve();
      }
    });
  });
}

function registerServiceWorker() {
  // The base dir used to resolve service_worker.js.
  navigator.serviceWorker.register('service_worker.js', {
    scope: './'
  }).then(swRegistrationReady).then(() => {
    sendResultToTest('ok - service worker registered');
  }).catch(sendErrorToTest);
}

// Query for the budget and return the current total.
function documentGetBudget() {
  navigator.budget.getBudget().then(function(budget) {
    sendResultToTest("ok - budget returned value of " + budget[0].budgetAt);
  }, function() {
    sendResultToTest("failed - unable to get budget values");
  });
}

// Request a reservation for a silent push.
function documentReserveBudget() {
  navigator.budget.reserve('silent-push').then(function(reserved) {
    if (reserved)
      sendResultToTest("ok - reserved budget");
    else
      sendResultToTest("failed - not able to reserve budget");
  }, function() {
    sendResultToTest("failed - error while trying to reserve budget");
  });
}

function workerGetBudget() {
  navigator.serviceWorker.controller.postMessage({command: 'workerGet'});
}

function workerReserveBudget() {
  navigator.serviceWorker.controller.postMessage({command: 'workerReserve'});
}

function isControlled() {
  if (navigator.serviceWorker.controller) {
    sendResultToTest('true - is controlled');
  } else {
    sendResultToTest('false - is not controlled');
  }
}

navigator.serviceWorker.addEventListener('message', function(event) {
  var message = JSON.parse(event.data);
  if (message.type == 'push')
    resultQueue.push(message.data);
  else
    sendResultToTest(message.data);
}, false);
