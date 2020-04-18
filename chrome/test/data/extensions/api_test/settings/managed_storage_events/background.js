// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests([
  function managedChangeEvents() {
    // This test enters twice to test storage.onChanged() notifications across
    // browser restarts for the managed namespace.
    //
    // The first run is when the extension is first installed; that run has
    // some initial policy that is verified, and then settings_apitest.cc
    // changes the policy to trigger an onChanged() event.
    //
    // The second run happens after the browser has been restarted, and the
    // policy is removed. Since this test extension has lazy background pages
    // it will start "idle", and the onChanged event should wake it up.
    //
    // |was_first_run| tracks whether onInstalled ever fired, so that the
    // onChanged() listener can probe for the correct policies.
    var was_first_run = false;

    // This only enters on PRE_ManagedStorageEvents, when the extension is
    // first installed.
    chrome.runtime.onInstalled.addListener(function() {
      // Verify initial policy.
      chrome.storage.managed.get(
          chrome.test.callbackPass(function(results) {
            chrome.test.assertEq({
              'constant-policy': 'aaa',
              'changes-policy': 'bbb',
              'deleted-policy': 'ccc'
            }, results);

            was_first_run = true;

            // Signal to the browser that the extension had performed the
            // initial load. The browser will change the policy and trigger
            // onChanged(). Note that this listener function is executed after
            // adding the onChanged() listener below.
            chrome.test.sendMessage('ready');
          }));
    });

    // Listen for onChanged() events.
    //
    // Note: don't use chrome.test.listenOnce() here! The onChanged() listener
    // must stay in place, otherwise the extension won't receive an event after
    // restarting!
    //
    // Manage the callbackAdded() callback manually to handle both runs of this
    // listener.
    var callbackCompleted = chrome.test.callbackAdded();
    chrome.storage.onChanged.addListener(function(changes, namespace) {
      var expectedChanges = {};
      if (was_first_run) {
        expectedChanges = {
          'changes-policy': {
            'oldValue': 'bbb',
            'newValue': 'ddd'
          },
          'deleted-policy': { 'oldValue': 'ccc' },
          'new-policy': { 'newValue': 'eee' }
        };
      } else {
        expectedChanges = {
          'changes-policy': { 'oldValue': 'ddd' },
          'constant-policy': { 'oldValue': 'aaa' },
          'new-policy': { 'oldValue': 'eee' }
        };
      }

      chrome.test.assertEq('managed', namespace);
      chrome.test.assertEq(expectedChanges, changes);

      callbackCompleted();
    });
  }
]);
