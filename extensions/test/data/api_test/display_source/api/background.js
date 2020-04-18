// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var testGetAvailableSinks = function() {
  var callback = function(sinks) {
    chrome.test.assertEq(1, sinks.length);
    var sink = sinks[0];
    chrome.test.assertEq(1, sink.id);
    chrome.test.assertEq("Disconnected", sink.state);
    chrome.test.assertEq("sink 1", sink.name);
    chrome.test.succeed("GetAvailableSinks succeeded");
  };
  chrome.displaySource.getAvailableSinks(callback);
};

var testOnSinksUpdated = function() {
  var callback = function(sinks) {
    chrome.test.assertEq(2, sinks.length);
    var sink = sinks[1];
    chrome.test.assertEq(2, sink.id);
    chrome.test.assertEq("Disconnected", sink.state);
    chrome.test.assertEq("sink 2", sink.name);
    chrome.test.succeed("onSinksUpdated event delivered");
  };
  chrome.displaySource.onSinksUpdated.addListener(callback);
};

var testRequestAuthentication = function() {
  var callback = function(auth_info) {
    chrome.test.assertEq("PIN", auth_info.method);
    chrome.test.assertEq(undefined, auth_info.data);
    chrome.test.succeed("RequestAuthentication succeeded");
  };
  chrome.displaySource.requestAuthentication(1, callback);
};

var testStartSessionErrorReport = function() {
  var callback = function() {
    chrome.test.assertLastError('Invalid stream arguments');
    chrome.test.succeed();
  };
  var session_info = { sinkId: 1, authenticationInfo: { method: "PBC" } };
  chrome.displaySource.startSession(session_info, callback);
};

var testTerminateSessionErrorReport = function() {
  var callback = function() {
    chrome.test.assertLastError('Session not found');
    chrome.test.succeed();
  };
  chrome.displaySource.terminateSession(1, callback);
};

chrome.test.runTests([testGetAvailableSinks,
                      testOnSinksUpdated,
                      testRequestAuthentication,
                      testStartSessionErrorReport,
                      testTerminateSessionErrorReport]);
