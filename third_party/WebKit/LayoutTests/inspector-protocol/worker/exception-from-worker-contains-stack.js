(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that console message from worker contains stack trace.');

  var workerRequestId = 1;
  function sendCommandToWorker(method, params, workerId) {
    dp.Target.sendMessageToTarget({
      targetId: workerId,
      message: JSON.stringify({ method: method, params: params, id: workerRequestId++ })
    });
  }

  var waitForWorkers = 2;
  dp.Target.onAttachedToTarget(messageObject => {
    var workerId = messageObject['params']['targetInfo']['targetId'];
    testRunner.log('Worker created');
    sendCommandToWorker('Runtime.enable', {}, workerId);
    if (!--waitForWorkers)
      session.evaluate('worker1.postMessage(239);worker2.postMessage(42);');
  });

  var workerTerminated = false;
  var messageReceived = false;
  dp.Target.onReceivedMessageFromTarget(messageObject => {
    var message = JSON.parse(messageObject['params']['message']);
    if (message['method'] === 'Runtime.exceptionThrown') {
      var callFrames = message.params.exceptionDetails.stackTrace ? message.params.exceptionDetails.stackTrace.callFrames : [];
      testRunner.log(callFrames.length > 0 ? 'Message with stack trace received.' : '[FAIL] Message contains empty stack trace');
      messageReceived = true;
      if (messageReceived && workerTerminated)
        testRunner.completeTest();
    }
  });

  function onDetached(messageObject) {
    dp.Target.offDetachedFromTarget(onDetached);
    workerTerminated = true;
    if (messageReceived && workerTerminated)
      testRunner.completeTest();
  }
  dp.Target.onDetachedFromTarget(onDetached);

  await dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: false});
  session.evaluate(`
    window.worker1 = new Worker('${testRunner.url('../resources/worker-with-throw.js')}');
    window.worker1.onerror = function(e) {
      e.preventDefault();
      worker1.terminate();
    }
    window.worker2 = new Worker('${testRunner.url('../resources/worker-with-throw.js')}');
  `);
})
