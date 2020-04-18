// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests toggling type filter on network panel.\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('network');

  NetworkTestRunner.recordNetwork();
  var target = UI.panels.network._networkLogView;
  var types = Common.resourceTypes;

  function toggleAndDump(buttonName, toggle) {
    TestRunner.addResult('');
    TestRunner.addResult((toggle ? 'Toggled \'' : 'Clicked \'') + buttonName + '\' button.');
    target._resourceCategoryFilterUI._toggleTypeFilter(buttonName, toggle);
    var results = [];
    var request = new SDK.NetworkRequest('', '', '', '', '');
    for (var typeId in types) {
      var type = Common.resourceTypes[typeId];
      results.push(type.name() + ': ' + target._resourceCategoryFilterUI.accept(type.category().title));
    }
    TestRunner.addResult('Filter: ' + results.join(', '));
  }

  var allTypes = 'all';

  toggleAndDump(allTypes, false);
  toggleAndDump(types.Document.category().title, false);
  toggleAndDump(types.Document.category().title, false);
  toggleAndDump(types.Script.category().title, false);

  toggleAndDump(allTypes, true);
  toggleAndDump(allTypes, true);
  toggleAndDump(types.Stylesheet.category().title, true);
  toggleAndDump(types.Image.category().title, true);
  toggleAndDump(types.Stylesheet.category().title, true);
  toggleAndDump(types.XHR.category().title, false);

  toggleAndDump(types.Font.category().title, true);
  toggleAndDump(types.WebSocket.category().title, true);
  toggleAndDump(types.Media.category().title, true);
  toggleAndDump(allTypes, false);

  TestRunner.completeTest();
})();
