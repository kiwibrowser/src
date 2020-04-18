// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.showPanel('network');

  var panel = UI.panels.network;
  var target = panel._networkLogView;
  var types = Common.resourceTypes;

  var requestFoo = new SDK.NetworkRequest('', '', '', '', '');
  requestFoo.setResourceType(types.XHR);
  requestFoo.setRequestIdForTest('foo');
  TestRunner.addResult('Showing request foo');
  panel._showRequest(requestFoo);
  TestRunner.addResult('Network Item View: ' + (panel._networkItemView && panel._networkItemView.isShowing()));

  TestRunner.addResult('Hiding request');
  eventSender.keyDown('Escape');
  await TestRunner.addSnifferPromise(Network.NetworkPanel.ActionDelegate.prototype, 'handleAction')
  TestRunner.addResult('Network Item View: ' + (panel._networkItemView && panel._networkItemView.isShowing()));

  TestRunner.completeTest();
})();