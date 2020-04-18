(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Test multiple retaining path for an object.`);

  await session.evaluate(`
    function run() {
      function leaking() {
        console.log('leaking');
      }
      var div = document.createElement('div');
      document.body.appendChild(div);
      div.addEventListener('click', leaking, true);
      document.body.addEventListener('click', leaking, true);
    }
    run();
  `);

  var Helper = await testRunner.loadScript('resources/heap-snapshot-common.js');
  var helper = await Helper(testRunner, session);

  var snapshot = await helper.takeHeapSnapshot();
  var node;
  for (var it = snapshot._allNodes(); it.hasNext(); it.next()) {
    if (it.node.type() === 'closure' && it.node.name() === 'leaking') {
      node = it.node;
      break;
    }
  }
  if (node)
    testRunner.log('SUCCESS: found ' + node.name());
  else
    return testRunner.fail('cannot find the leaking node');

  var eventListener = helper.firstRetainingPath(node)[0];

  if (eventListener.name() == 'EventListener') {
    testRunner.log('SUCCESS: immediate retainer is EventListener.');
  } else {
    return testRunner.fail('cannot find the EventListener.');
  }

  var retainingPaths = [];
  for (var iter = eventListener.retainers(); iter.hasNext(); iter.next()) {
    var path = helper.firstRetainingPath(iter.retainer.node());
    path = path.map(node => node.name());
    retainingPaths.push(path.join(', '));
  }

  if (retainingPaths.length >= 2) {
    testRunner.log('SUCCESS: found multiple retaining paths.');
  } else {
    return testRunner.fail('cannot find multiple retaining paths.');
  }

  // Sort alphabetically to make the test robust.
  retainingPaths.sort((a, b) => (a < b ? -1 : (a == b ? 0 : 1)));
  testRunner.log(`SUCCESS: path1 = [${retainingPaths[0]}]`);
  testRunner.log(`SUCCESS: path2 = [${retainingPaths[1]}]`);
  testRunner.completeTest();
})
