// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var expectedEventData;
var capturedEventData;

function expect(data) {
  expectedEventData = data;
  capturedEventData = [];
}

// Claim clients to send postMessage reply to them.
self.addEventListener('activate', function(e) {
  e.waitUntil(self.clients.claim());
});

function sendMessage(msg) {
  clients.matchAll({}).then(function(clients) {
    clients.forEach(function(client) {
      client.postMessage(msg);
    });
  });
}

function checkExpectations() {
  if (capturedEventData.length < expectedEventData.length) {
    return;
  }

  var passed = JSON.stringify(expectedEventData) ==
      JSON.stringify(capturedEventData);
  if (passed) {
    sendMessage('chrome.tabs.onUpdated callback');
  } else {
    sendMessage('FAILURE');
  }
}

function addOnUpdatedListener() {
  chrome.tabs.onUpdated.addListener(function(tabId, info, tab) {
    capturedEventData.push(info);
    checkExpectations();
  });

  var url = chrome.extension.getURL('on_updated.html');
  expect([
    {status: 'loading', 'url': url},
    {status: 'complete'},
    {title: 'foo'},
    {title: 'bar'}
  ]);
};

addOnUpdatedListener();
