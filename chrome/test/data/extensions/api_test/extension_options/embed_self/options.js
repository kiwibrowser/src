// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This script is opened inside a <extensionoptions> guest view.

// This adds a 'loaded' property to the window for the createGuestViewDOM and
// createGuestViewProgrammatic tests.
window.loaded = true;

// This sends a message back to the test page, which responds with which test
// case it is running. The options page then runs the appropriate code for the
// specified test case.
chrome.runtime.sendMessage('ready', function(command) {
  switch (command) {
    case 'canCommunicateWithGuest':
      // To confirm that the guest view has been successfully created,
      // {pass: true} is added to every extension Window and broadcasts a
      // message to the extension using runtime.sendMessage().
      chrome.extension.getViews().forEach(function(view) {
        view.pass = true;
      });
      chrome.runtime.sendMessage('done');
      break;

    case 'guestCanAccessStorage':
      // To test access to privileged APIs, the guest attempts to write to local
      // storage. The guest relays the callbacks for storage.onChanged and
      // storage.set to the test runner for verification.
      chrome.storage.onChanged.addListener(function(change) {
        chrome.runtime.sendMessage({
          expected: 42,
          actual: change.test.newValue,
          description: 'onStorageChanged'
        });
      });

      chrome.storage.local.set({'test': 42}, function() {
        chrome.storage.local.get('test', function(storage) {
          chrome.runtime.sendMessage({
            expected: 42,
            actual: storage.test,
            description: 'onSetAndGet'
          });
        });
      });

      break;

    case 'externalLinksOpenInNewTab':
      var link = document.getElementById('link');
      chrome.test.runWithUserGesture(function() {
        link.click();
        chrome.runtime.sendMessage('done');
      }.bind(link));
  }
});
