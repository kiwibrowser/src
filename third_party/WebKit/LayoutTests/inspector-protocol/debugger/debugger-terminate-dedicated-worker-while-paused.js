(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Test that inspected page won't crash if inspected worker is terminated while it is paused. Test passes if it doesn't crash. Bug 101065.`);

  await session.evaluate(`
    window.worker = new Worker('${testRunner.url('resources/dedicated-worker.js')}');
    window.worker.onmessage = function(event) { };
    window.worker.postMessage(1);
  `);
  testRunner.log('Started worker');

  var workerRequestId = 1;
  function sendCommandToWorker(method, params) {
    var message = {method, params, id: workerRequestId};
    dp.Target.sendMessageToTarget({targetId: workerId, message: JSON.stringify(message)});
    return workerRequestId++;
  }

  dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: false});

  var messageObject = await dp.Target.onceAttachedToTarget();
  var workerId = messageObject.params.targetInfo.targetId;
  testRunner.log('Worker created');
  testRunner.log('didConnectToWorker');
  sendCommandToWorker('Debugger.enable', {});
  sendCommandToWorker('Debugger.pause', {});

  dp.Target.onReceivedMessageFromTarget(async messageObject => {
    var message = JSON.parse(messageObject.params.message);
    if (message.method === 'Debugger.paused') {
      testRunner.log('Worker paused');
      await dp.Runtime.evaluate({expression: 'worker.terminate()' });
      testRunner.log('SUCCESS: Did terminate paused worker');
      testRunner.completeTest();
    }
  });
})
