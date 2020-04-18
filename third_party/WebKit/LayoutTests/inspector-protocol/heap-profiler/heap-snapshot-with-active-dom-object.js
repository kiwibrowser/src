(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
    `Test that all ActiveDOMObjects with pending activities will get into one group in the heap snapshot. Bug 426809.`);

  await session.evaluate(`
    function onChange(e) {
      console.log('onChange ' + e.matches);
    }
    var m = window.matchMedia('(min-width: 1400px)');
    m.addListener(onChange);
    m = window.matchMedia('(min-height: 1800px)');
    m.addListener(onChange);
    console.log('Created 2 MediaQueryList elements');
  `);

  function checkPendingActivities(groupNode) {
    var mediaQuryListCount = 0;
    for (var iter = groupNode.edges(); iter.hasNext(); iter.next()) {
      var node = iter.edge.node();
      if (node.name() === 'MediaQueryList')
        ++mediaQuryListCount;
    }
    if (mediaQuryListCount === 2)
      testRunner.log('SUCCESS: found ' + mediaQuryListCount + ' MediaQueryLists in ' + groupNode.name());
    else
      return testRunner.fail('unexpected MediaQueryLists count: ' + mediaQuryListCount);
  }

  var Helper = await testRunner.loadScript('resources/heap-snapshot-common.js');
  var helper = await Helper(testRunner, session);

  var snapshot = await helper.takeHeapSnapshot();
  var node;
  for (var it = snapshot._allNodes(); it.hasNext(); it.next()) {
    if (it.node.name() === 'Pending activities') {
      node = it.node;
      break;
    }
  }
  if (node)
    testRunner.log('SUCCESS: found ' + node.name());
  else
    return testRunner.fail(`cannot find 'Pending activities'`);

  checkPendingActivities(node);

  testRunner.completeTest();
})
