(async function() {
  runtime._registerModule({
    name: "mock-module",
    extensions: ['first', 'second', 'third', 'fourth'].map(title => {
      return {
        "type": "view",
        "location": "mock-location",
        "id": title,
        "title": title,
        "persistence": "closeable",
        "factoryName": "UI.Widget"
      }
    }),
    scripts: []
  });

  var tabbedLocation;
  var viewManager;
  createTabbedLocation();
  dumpTabs();
  TestRunner.addResult('Appending three views')
  viewManager.showView('first');
  viewManager.showView('second');
  viewManager.showView('third');
  dumpTabs();
  createTabbedLocation();
  dumpTabs();
  TestRunner.addResult('Re-order tabs');
  tabbedLocation.tabbedPane()._insertBefore(tabbedLocation.tabbedPane()._tabsById.get("third"), 0);
  dumpTabs();
  createTabbedLocation();
  dumpTabs();
  viewManager.showView('fourth');
  dumpTabs();
  createTabbedLocation();
  dumpTabs();
  TestRunner.addResult('Closing second tab');
  tabbedLocation.tabbedPane().closeTab('second');
  dumpTabs();
  createTabbedLocation();
  dumpTabs();
  TestRunner.completeTest();

  function createTabbedLocation() {
    TestRunner.addResult('Creating new TabbedLocation');
    if (tabbedLocation)
      tabbedLocation.tabbedPane().detach(true);
    viewManager = new UI.ViewManager();
    tabbedLocation = viewManager.createTabbedLocation(undefined, 'mock-location', true, true);
    tabbedLocation.widget().show(UI.inspectorView.element);
  }

  function dumpTabs() {
    TestRunner.addResult(JSON.stringify(tabbedLocation.tabbedPane().tabIds()));
  }
})();
