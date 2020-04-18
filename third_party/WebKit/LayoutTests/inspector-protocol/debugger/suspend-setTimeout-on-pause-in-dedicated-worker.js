(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that setTimeout callback will not fire while script execution is paused.Bug 377926.');

  var workerId;
  var workerRequestId = 1;
  function sendCommandToWorker(method, params) {
    var message = {method, params, id: workerRequestId};
    dp.Target.sendMessageToTarget({targetId: workerId, message: JSON.stringify(message)});
    return workerRequestId++;
  }

  dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: true});
  await session.evaluate(`
    window.worker = new Worker('${testRunner.url('resources/dedicated-worker-suspend-setTimeout.js')}');
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
  var evalRequestId;
  dp.Target.onReceivedMessageFromTarget(async messageObject => {
    var message = JSON.parse(messageObject.params.message);
    if (message.method === 'Debugger.paused') {
      testRunner.log('SUCCESS: Worker paused');
      if (++pauseCount === 1) {
        evalRequestId = sendCommandToWorker('Runtime.evaluate', {expression: 'global_value'});
      } else {
        testRunner.log('FAIL: debugger paused second time');
        testRunner.completeTest();
      }
    } else if (evalRequestId && message.id === evalRequestId) {
      var value = message.result.result.value;
      if (value === 1)
        testRunner.log('SUCCESS: global_value is 1');
      else
        testRunner.log('FAIL: setTimeout callback fired while script execution was paused');
      sendCommandToWorker('Debugger.disable', {});
      testRunner.completeTest();
    }
  });
})
