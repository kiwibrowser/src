// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var expectedEventData;
var capturedEventData;

function expect(data) {
  expectedEventData = data;
  capturedEventData = [];
}

var messagePort = null;
function sendMessage(msg) { messagePort.postMessage(msg); }

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

self.onmessage = function(e) {
  var data = e.data;
  messagePort = e.ports[0];
  if (data == 'addOnUpdatedListener') {
    addOnUpdatedListener();
    e.ports[0].postMessage('listener-added');
  }
};

function addOnUpdatedListener() {
  chrome.tabs.onUpdated.addListener(function(tabId, info, tab) {
    console.log('onUpdated, tabId: ' + tabId + ', info: ' +
        JSON.stringify(info) + ', tab: ' + JSON.stringify(tab));
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
