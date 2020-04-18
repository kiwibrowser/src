// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests fetch network filters\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('network');

  await NetworkTestRunner.clearNetworkCache();

  NetworkTestRunner.recordNetwork();

  NetworkTestRunner.makeFetch('resources/style.css', {}, ensureAllResources);
  NetworkTestRunner.makeFetch('resources/abe.png', {}, () => {
    // Ensures result is cached.
    NetworkTestRunner.makeFetch('resources/abe.png', {}, ensureAllResources);
    ensureAllResources();
  });
  NetworkTestRunner.makeFetch('missing/foo.bar', {}, ensureAllResources);

  var filterChecks = [
    '-.css',
    '-.png',
    'css',
    '',
    '/.*/',
    '/.*\\..*/',
    '/.*\\.png/',
    '/NOTHINGTOMATCH/',
    '//',
    'png',
    '-missing',
    'is:from-cache',
    '-is:from-cache',
  ];

  var resourceCount = 0;
  var totalResourceCount = 4;
  function ensureAllResources() {
    if (++resourceCount >= totalResourceCount)
      checkFilters();
  }

  function checkFilters() {
    for (var filterText of filterChecks) {
      TestRunner.addResult('filterText: ' + filterText);
      setNetworkLogFilter(filterText);

      var nodes = UI.panels.network._networkLogView.flatNodesList();
      var foundNodesCount = 0;
      for (var i = 0; i < nodes.length; i++) {
        if (!nodes[i][Network.NetworkLogView._isFilteredOutSymbol])
          foundNodesCount++;
      }

      TestRunner.addResult('Found results: ' + foundNodesCount);
      TestRunner.addResult('');
    }
    TestRunner.completeTest();
  }

  /**
     * @param {string} value
     */
  function setNetworkLogFilter(value) {
    UI.panels.network._networkLogView._textFilterUI.setValue(value);
    UI.panels.network._networkLogView._filterChanged(null);  // event not used in this method, so passing null
  }
})();
