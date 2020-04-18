// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var api = chrome.storage;
var assertEq = chrome.test.assertEq;
var inIncognitoContext = chrome.extension.inIncognitoContext;

['sync', 'local'].forEach(function(namespace) {
  api[namespace].notifications = {};
  api.onChanged.addListener(function(changes, event_namespace) {
    if (event_namespace == namespace) {
      var notifications = api[namespace].notifications;
      Object.keys(changes).forEach(function(key) {
        notifications[key] = changes[key];
      });
    }
  });
});

// The test from C++ runs "actions", where each action is defined here.
// This allows the test to be tightly controlled between incognito and
// non-incognito modes.
// Each function accepts a callback which should be run when the settings
// operation fully completes.
var testActions = {
  noop: function(callback) {
    this.get("", callback);
  },
  assertEmpty: function(callback) {
    this.get(null, function(settings) {
      assertEq({}, settings);
      callback();
    });
  },
  assertFoo: function(callback) {
    this.get(null, function(settings) {
      assertEq({foo: "bar"}, settings);
      callback();
    });
  },
  setFoo: function(callback) {
    this.set({foo: "bar"}, callback);
  },
  removeFoo: function(callback) {
    this.remove("foo", callback);
  },
  clear: function(callback) {
    this.clear(callback);
  },
  assertNoNotifications: function(callback) {
    assertEq({}, this.notifications);
    callback();
  },
  clearNotifications: function(callback) {
    this.notifications = {};
    callback();
  },
  assertAddFooNotification: function(callback) {
    assertEq({ foo: { newValue: 'bar' } }, this.notifications);
    callback();
  },
  assertDeleteFooNotification: function(callback) {
    assertEq({ foo: { oldValue: 'bar' } }, this.notifications);
    callback();
  }
};

// The only test we run.  Runs "actions" (as defined above) until told
// to stop (when the message has isFinalAction set to true).
function testEverything() {
  function next() {
    var waiting = inIncognitoContext ? "waiting_incognito" : "waiting";
    chrome.test.sendMessage(waiting, function(messageJson) {
      var message = JSON.parse(messageJson);
      var action = testActions[message.action];
      if (!action) {
        chrome.test.fail("Unknown action: " + message.action);
        return;
      }
      action.bind(api[message.namespace])(
          message.isFinalAction ? chrome.test.succeed : next);
    });
  }
  next();
}

chrome.test.runTests([testEverything]);
