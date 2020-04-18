(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that closing the page will resume the render process.`);

  await dp.Target.setDiscoverTargets({discover: true});
  session.evaluate(`
    window.myWindow = window.open('../resources/inspector-protocol-page.html'); undefined;
  `);
  testRunner.log('Opened a second window');

  var event = await dp.Target.onceTargetInfoChanged(event => event.params.targetInfo.url.endsWith('inspector-protocol-page.html'));
  var targetId = event.params.targetInfo.targetId;

  var sessionId = (await dp.Target.attachToTarget({targetId: targetId})).result.sessionId;
  testRunner.log('Attached to a second window');
  await dp.Target.sendMessageToTarget({
    sessionId: sessionId,
    message: JSON.stringify({id: 1, method: 'Debugger.enable'})
  });
  dp.Target.sendMessageToTarget({
    sessionId: sessionId,
    message: JSON.stringify({id: 2, method: 'Runtime.evaluate', params: {expression: 'debugger;'}})
  });
  await dp.Target.onceReceivedMessageFromTarget(event => {
    var message = JSON.parse(event.params.message);
    return message.method === 'Debugger.paused';
  });
  testRunner.log('Paused in a second window');

  session.evaluate(`
    window.myWindow.close(); undefined;
  `);
  testRunner.log('Closed a second window');
  await dp.Target.onceTargetDestroyed(event => event.params.targetId === targetId);
  testRunner.log('Received window destroyed notification');

  await session.evaluateAsync(`
    new Promise(f => setTimeout(f, 0))
  `);
  testRunner.log('setTimeout worked in first window');
  testRunner.completeTest();
})
