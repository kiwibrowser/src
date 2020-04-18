// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests list of user metrics codes and invocations.\n`);
  await TestRunner.loadModule('performance_test_runner');
  await TestRunner.loadModule('cpu_profiler_test_runner');

  InspectorFrontendHost.recordEnumeratedHistogram = function(name, code) {
    if (name === 'DevTools.ActionTaken')
      TestRunner.addResult('Action taken: ' + nameOf(Host.UserMetrics.Action, code));
    else if (name === 'DevTools.PanelShown')
      TestRunner.addResult('Panel shown: ' + nameOf(Host.UserMetrics._PanelCodes, code));
  };

  function nameOf(object, code) {
    for (var name in object) {
      if (object[name] === code)
        return name;
    }
    return null;
  }

  TestRunner.addResult('recordActionTaken:');
  TestRunner.dump(Host.UserMetrics.Action);
  Host.userMetrics.actionTaken(Host.UserMetrics.Action.WindowDocked);
  Host.userMetrics.actionTaken(Host.UserMetrics.Action.WindowUndocked);

  TestRunner.addResult('\nrecordPanelShown:');
  TestRunner.dump(Host.UserMetrics._PanelCodes);
  UI.viewManager.showView('js_profiler');
  UI.viewManager.showView('timeline');

  TestRunner.addResult('\nTest that drawer usage is recorded by PanelShown');
  TestRunner.addResult('Show drawer:');
  UI.inspectorView._showDrawer(true);
  TestRunner.addResult('Selecting tab from triple dots menu:');
  UI.inspectorView._drawerTabbedLocation.showView(UI.inspectorView._drawerTabbedLocation._views.get("animations"), undefined, true);
  TestRunner.addResult('Selecting tab from tabbed pane header:');
  UI.inspectorView._drawerTabbedLocation._tabbedPane._tabs[0]._tabElement.dispatchEvent(new MouseEvent("mousedown"));

  TestRunner.completeTest();
})();
