// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const notifications = chrome.notifications;

var idString = "foo";

var testBasicEvents = function() {
  var incidents = 0;

  var onCreateCallback = function(id) {
    chrome.test.assertTrue(id.length > 0);
    chrome.test.assertEq(idString, id);
    chrome.test.succeed();
  }

  var red_dot = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUA" +
      "AAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO" +
      "9TXL0Y4OHwAAAABJRU5ErkJggg==";

  var options = {
    type: "basic",
    iconUrl: red_dot,
    title: "Attention!",
    message: "Check out Cirque du Soleil"
  };
  notifications.create(idString, options, onCreateCallback);
};

chrome.test.runTests([ testBasicEvents ]);
