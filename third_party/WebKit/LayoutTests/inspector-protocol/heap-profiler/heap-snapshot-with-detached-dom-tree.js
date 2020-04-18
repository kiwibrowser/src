(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Test that all nodes from the detached DOM tree will be marked as detached.`);
  await session.evaluate(`
    window.retaining_wrapper = document.createElement('div');
    var t = document.createElement('div');
    retaining_wrapper.appendChild(t);
    t.appendChild(document.createElement('div'));
  `);
  var Helper = await testRunner.loadScript('resources/heap-snapshot-common.js');
  var helper = await Helper(testRunner, session);
  var snapshot = await helper.takeHeapSnapshot();
  var divCount = 0;
  for (var it = snapshot._allNodes(); it.hasNext(); it.next()) {
    if (it.node.name() === 'Detached HTMLDivElement')
      ++divCount;
  }
  if (divCount === 3)
    testRunner.log('SUCCESS: found 3 detached DIV elements');
  else
    return testRunner.fail('unexpected detached DIV count: ' + divCount);
  testRunner.completeTest();
})