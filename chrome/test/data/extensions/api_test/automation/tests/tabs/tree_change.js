// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var allTests = [
  function testTreeChangedObserverForCreatingNode() {
    chrome.automation.addTreeChangeObserver("allTreeChanges", function(change) {
      if (change.type == "subtreeCreated" && change.target.name == "New") {
        chrome.test.succeed();
      }
    });

    var addButton = rootNode.find({ attributes: { name: 'Add' }});
    addButton.doDefault();
  },

  function testTreeChangedObserverForRemovingNode() {
    chrome.automation.addTreeChangeObserver("allTreeChanges", function(change) {
      if (change.type == "nodeRemoved" && change.target.role == "listItem") {
        chrome.test.succeed();
      }
    });

    var removeButton = rootNode.find({ attributes: { name: 'Remove' }});
    removeButton.doDefault();
  },

  function testTreeChangedObserverForLiveRegionsOnly() {
    // This test would fail if we set the filter to allTreeChanges.
    chrome.automation.addTreeChangeObserver(
        "liveRegionTreeChanges",
        function(change) {
      if (change.target.name == 'Dead') {
        chrome.test.fail();
      }
      if (change.target.name == 'Live') {
        chrome.test.succeed();
      }
    });

    var liveButton = rootNode.find({ attributes: { name: 'Live' }});
    liveButton.doDefault();
  }

];

setUpAndRunTests(allTests, 'tree_change.html');
