(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests that dedicated worker won't crash on attempt to step into.Bug 232392.`);

  var workerId;
  var workerRequestId = 1;
  function sendCommandToWorker(method, params) {
    var message = {method, params, id: workerRequestId};
    dp.Target.sendMessageToTarget({targetId: workerId, message: JSON.stringify(message)});
    return workerRequestId++;
  }

  dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: true});
  await session.evaluate(`
    window.worker = new Worker('${testRunner.url('resources/dedicated-worker-step-into.js')}');
    window.worker.onmessage = function(event) { };
    window.worker.postMessage(1);
  `);
  testRunner.log('Started worker');

  var messageObject = await dp.Target.onceAttachedToTarget();
  workerId = messageObject.params.targetInfo.targetId;
  testRunner.log('Worker created');

  sendCommandToWorker('Debugger.enable', {});
  sendCommandToWorker('Runtime.runIfWaitingForDebugger', {});

  var pauseCount = 0;
  dp.Target.onReceivedMessageFromTarget(async messageObject => {
    var message = JSON.parse(messageObject.params.message);
    if (message.method === 'Debugger.paused') {
      testRunner.log('SUCCESS: Worker paused');
      if (++pauseCount === 1) {
        testRunner.log('Stepping into...');
        sendCommandToWorker('Debugger.stepInto', {});
      } else {
        sendCommandToWorker('Debugger.disable', {});
        testRunner.completeTest();
      }
    }
  });
})
