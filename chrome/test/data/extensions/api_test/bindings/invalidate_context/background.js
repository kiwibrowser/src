// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var frame;
var frameRuntime;
var frameStorage;
var frameTabs;

function createFrame() {
  frame = document.createElement('iframe');
  frame.src = chrome.runtime.getURL('frame.html');
  return new Promise((resolve) => {
    frame.onload = resolve;
    document.body.appendChild(frame);
  });
}

function testPort(port) {
  var result = {
    disconnectThrow: false,
    postMessageThrow: false,
    onMessageEvent: undefined,
    onDisconnectEvent: undefined,
    getOnMessageThrow: false,
    getOnDisconnectThrow: false,
  };

  try {
    port.postMessage('hello');
  } catch (e) {
    result.postMessageThrow = true;
  }

  try {
    result.onMessageEvent = port.onMessage;
  } catch (e) {
    result.getOnMessageThrow = true;
  }

  try {
    result.onDisconnect = port.onDisconnect;
  } catch (e) {
    result.getOnDisconnectThrow = true;
  }

  try {
    port.disconnect();
  } catch (e) {
    result.disconnectThrow = true;
  }

  chrome.test.assertTrue(result.postMessageThrow);
  chrome.test.assertTrue(result.disconnectThrow);
  chrome.test.assertTrue(result.getOnMessageThrow);
  chrome.test.assertTrue(result.getOnDisconnectThrow);
  chrome.test.assertFalse(!!result.onMessageEvent);
  chrome.test.assertFalse(!!result.onDisconnectEvent);
}

chrome.test.runTests([
  function useFrameStorageAndRuntime() {
    createFrame().then(() => {
      frameRuntime = frame.contentWindow.chrome.runtime;
      chrome.test.assertTrue(!!frameRuntime);
      frameStorage = frame.contentWindow.chrome.storage.local;
      chrome.test.assertTrue(!!frameStorage);
      frameTabs = frame.contentWindow.chrome.tabs;
      chrome.test.assertTrue(!!frameTabs);
      chrome.test.assertEq(chrome.runtime.getURL('background.js'),
                           frameRuntime.getURL('background.js'));
      frameStorage.set({foo: 'bar'}, function() {
        chrome.test.assertFalse(!!chrome.runtime.lastError);
        chrome.test.assertFalse(!!frameRuntime.lastError);
        chrome.storage.local.get('foo', function(vals) {
          chrome.test.assertFalse(!!chrome.runtime.lastError);
          chrome.test.assertFalse(!!frameRuntime.lastError);
          chrome.test.assertEq('bar', vals.foo);
          chrome.test.succeed();
        });
      });
    });
  },
  function removeFrameAndUseStorageAndRuntime() {
    document.body.removeChild(frame);
    try {
      frameStorage.set({foo: 'baz'});
    } catch (e) {}

    try {
      let url = frameRuntime.getURL('background.js');
    } catch (e) {}

    chrome.test.succeed();
  },
  function usePortAfterInvalidation() {
    var listener = function() {};
    chrome.runtime.onConnect.addListener(listener);
    createFrame().then(() => {
      var frameRuntime = frame.contentWindow.chrome.runtime;
      var port = frameRuntime.connect();
      chrome.test.assertTrue(!!port);

      port.postMessage;
      document.body.removeChild(frame);

      testPort(port);
      chrome.test.succeed();
    });
  },
  function usePortAfterInvalidationAndDisconnect() {
    createFrame().then(() => {
      var frameRuntime = frame.contentWindow.chrome.runtime;
      var port = frameRuntime.connect();
      chrome.test.assertTrue(!!port);

      port.disconnect();
      document.body.removeChild(frame);

      testPort(port);
      chrome.test.succeed();
    });
  },
  function usePortAfterInvalidationAndEventsCreated() {
    createFrame().then(() => {
      var frameRuntime = frame.contentWindow.chrome.runtime;
      var port = frameRuntime.connect();
      chrome.test.assertTrue(!!port);

      port.onMessage;
      port.onDisconnect;
      document.body.removeChild(frame);

      testPort(port);
      chrome.test.succeed();
    });
  },
]);
