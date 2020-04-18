// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Don't wait for clients of old SW to close before activating.
self.addEventListener('install', () => skipWaiting());

// Accept messages from the test JavaScript to trigger worker based tests.
self.addEventListener('message', function (event) {
  if (event.data.command == 'workerGet') {
    workerGetBudget();
  } else if (event.data.command == 'workerReserve') {
    workerReserve();
  } else {
    sendMessageToClients('message', 'error - unknown message request');
    return;
  }
});

// Query for the budget and return the current total.
function workerGetBudget() {
  navigator.budget.getBudget().then(function(budget) {
    sendMessageToClients('message',
        'ok - budget returned value of ' + budget[0].budgetAt);
  }, function() {
    sendMessageToClients('message', 'failed - unable to get budget values');
  });
}

// Request a reservation for a silent push.
function workerReserve() {
  navigator.budget.reserve('silent-push').then(function(reserved) {
    if (reserved)
      sendMessageToClients('message', 'ok - reserved budget');
    else
      sendMessageToClients('message', 'failed - not able to reserve budget');
  }, function() {
    sendMessageToClients('message',
        'failed - error while trying to reserve budget');
  });
}

function sendMessageToClients(type, data) {
  const message = JSON.stringify({
    'type': type,
    'data': data
  });
  clients.matchAll().then(function(clients) {
    clients.forEach(function(client) {
      client.postMessage(message);
    });
  }, function(error) {
    console.log(error);
  });
}
