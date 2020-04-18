// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function setAnalyticsComponentLight() {
  chrome.mediaPerceptionPrivate.setAnalyticsComponent({
    type: 'LIGHT',
  }, chrome.test.callbackPass(function(component_state) {
    chrome.test.assertEq('INSTALLED', component_state.status);
    chrome.test.assertEq('1.0', component_state.version);
  }));
}

function setAnalyticsComponentFullExpectFailure() {
  chrome.mediaPerceptionPrivate.setAnalyticsComponent({
    type: 'FULL',
  }, chrome.test.callbackPass(function(component_state) {
    chrome.test.assertEq('FAILED_TO_INSTALL', component_state.status);
  }));
}

function setAnalyticsComponentWithProcessRunningFailure() {
  chrome.mediaPerceptionPrivate.setState({
    status: 'RUNNING',
    deviceContext: 'device_context'
  }, chrome.test.callbackPass(function(state) {
    chrome.test.assertEq('RUNNING', state.status);
  }));

  chrome.mediaPerceptionPrivate.setAnalyticsComponent({
    type: 'LIGHT',
  }, chrome.test.callbackPass(function(component_state) {
    chrome.test.assertEq('FAILED_TO_INSTALL', component_state.status);
  }));
}

chrome.test.runTests([
    setAnalyticsComponentLight,
    setAnalyticsComponentFullExpectFailure,
    setAnalyticsComponentWithProcessRunningFailure]);

