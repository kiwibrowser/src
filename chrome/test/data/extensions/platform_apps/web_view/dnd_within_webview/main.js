// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.lastDropData = 'Uninitialized';

var LOG = function(msg) {
  window.console.log(msg);
};

// Whether or not we were asked by cpp to read lastDropData.
var isCheckingIfGuestGotDrop = false;
// Whether or not guest has seen a 'drop' operation in dnd.
var gotDropFromGuest = false;
var sentConnectedMsg = false;
var sentGuestGotDropMsg = false;

// Called from web_view_interactive_ui_tests.cc.
window.checkIfGuestGotDrop = function() {
  LOG('embedder.window.checkIfGuestGotDrop()');
  isCheckingIfGuestGotDrop = true;
  maybeSendGuestGotDropMsg();
};

// Called from web_view_interactive_ui_tests.cc.
window.resetState = function() {
  LOG('embedder.window.resetState');
  isCheckingIfGuestGotDrop = false;
  sentGuestGotDropMsg = false;
  gotDropFromGuest = false;
  webview.contentWindow.postMessage(JSON.stringify(['resetState']), '*');
};

// Called from web_view_interactive_ui_tests.cc.
window.getLastDropData = function() {
  LOG('getLastDropData: returns [' + lastDropData + ']');
  return window.lastDropData;
};

var maybeSendGuestGotDropMsg = function() {
  LOG('maybeSendGuestGotDropMsg, ' +
      'isCheckingIfGuestGotDrop: ' + isCheckingIfGuestGotDrop +
      ', sentGuestGotDropMsg: ' + sentGuestGotDropMsg +
      ', gotDropFromGuest: ' + gotDropFromGuest);
  if (isCheckingIfGuestGotDrop && !sentGuestGotDropMsg && gotDropFromGuest) {
    sentGuestGotDropMsg = true;
    chrome.test.sendMessage('guest-got-drop');
  }
};

var startTest = function() {
  var guestURL = 'about:blank';  // Actual contents are injected below.
  document.querySelector('#webview-tag-container').innerHTML =
      '<webview id=\'webview\' style="width: 300px; height: 150px; ' +
      'margin: 0; padding: 0;"' +
      ' src="' + guestURL + '"' +
      '></webview>';

  window.addEventListener('message', onPostMessageReceived, false);
  var webview = document.getElementById('webview');
  webview.addEventListener('loadstop', function(e) {
    LOG('loadstop');
    webview.insertCSS({file: 'guest.css'}, function() {
      LOG('insertCSS response.');
      webview.executeScript({file: 'guest.js'}, function(results) {
        LOG('executeScript response, length: ' + results.length);
        webview.contentWindow.postMessage(
            JSON.stringify(['create-channel']), '*');
      });
    });
  });

  // For debug messages from guests.
  webview.addEventListener('consolemessage', function(e) {
    LOG('[Guest]: ' + e.message);
  });
};

var onPostMessageReceived = function(e) {
  LOG('embedder.onPostMessageReceived');
  var data = JSON.parse(e.data);
  LOG('data: ' + data);
  switch (data[0]) {
    case 'connected':
      // Trigger a resize event on the guest so that we make sure
      // we are painted before we attempt to start drag/drop test.
      document.getElementById('webview').style.height = '200px';
      break;
    case 'resized':
      LOG('resized msg, sentConnectedMsg: ' + sentConnectedMsg);
      if (!sentConnectedMsg) {
        sentConnectedMsg = true;
        chrome.test.sendMessage('connected');
      }
      break;
    case 'guest-got-drop':
      window.lastDropData = data[1];
      gotDropFromGuest = true;
      maybeSendGuestGotDropMsg();
      break;
    case 'resetStateReply':
      chrome.test.sendMessage('resetStateReply');
      break;
    default:
      LOG('ERR: curious message received in emb: ' + data);
      break;
  }
};

startTest();
