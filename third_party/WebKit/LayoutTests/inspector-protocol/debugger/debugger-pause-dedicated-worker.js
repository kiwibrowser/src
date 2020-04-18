(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that worker can be paused.');

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

  dp.Target.onReceivedMessageFromTarget(messageObject => {
    var message = JSON.parse(messageObject.params.message);
    if (message.method === 'Debugger.paused') {
      testRunner.log('SUCCESS: Worker paused');
      sendCommandToWorker('Debugger.disable', {});
      testRunner.completeTest();
    }
  });
})
