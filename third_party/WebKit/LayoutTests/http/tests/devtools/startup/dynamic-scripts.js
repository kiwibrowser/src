// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  await TestRunner.setupStartupTest('resources/dynamic-scripts.html');
  TestRunner.addResult(
      `Tests that scripts for dynamically added script elements are shown in sources panel if inspector is opened after the scripts were loaded. https://bugs.webkit.org/show_bug.cgi?id=99324\n`);
  await TestRunner.loadModule('sources_test_runner');

  SourcesTestRunner.startDebuggerTest(step2);

  function step2() {
    TestRunner.deprecatedRunAfterPendingDispatches(step3);
  }

  function step3() {
    var panel = UI.panels.sources;
    var uiSourceCodes = Workspace.workspace.uiSourceCodesForProjectType(Workspace.projectTypes.Network);
    var urls = uiSourceCodes.map(function(uiSourceCode) {
      return uiSourceCode.name();
    });
    urls.sort();

    var whiteList = [
      'dynamic-script.js', 'dynamic-scripts.html', 'evalSourceURL.js', 'scriptElementContentSourceURL.js'
    ];
    function filter(url) {
      for (var i = 0; i < whiteList.length; ++i) {
        if (url.indexOf(whiteList[i]) !== -1)
          return true;
      }

      return false;
    }
    urls = urls.filter(filter);

    TestRunner.addResult('UISourceCodes:');
    for (var i = 0; i < urls.length; ++i)
      TestRunner.addResult('    ' + urls[i]);
    SourcesTestRunner.completeDebuggerTest();
  }
})();
