(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests sourceURL in setTimeout from worker.');

  await session.evaluate(`
    window.worker = new Worker('${testRunner.url('resources/dedicated-worker-string-setTimeout.js')}');
    window.worker.onmessage = function(event) { };
    window.worker.postMessage(1);
  `);
  testRunner.log('Started worker');

  var workerId;
  var workerRequestId = 1;
  function sendCommandToWorker(method, params) {
    var message = {method, params, id: workerRequestId};
    dp.Target.sendMessageToTarget({targetId: workerId, message: JSON.stringify(message)});
    return workerRequestId++;
  }

  dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: false});

  var messageObject = await dp.Target.onceAttachedToTarget();
  workerId = messageObject.params.targetInfo.targetId;
  testRunner.log('Worker created');
  testRunner.log('didConnectToWorker');

  var debuggerEnableRequestId = sendCommandToWorker('Debugger.enable', {});
  var postMessageToWorker = false;
  dp.Target.onReceivedMessageFromTarget(async messageObject => {
    var message = JSON.parse(messageObject.params.message);
    if (message.id === debuggerEnableRequestId) {
      testRunner.log('Did enable debugger');
      // Start setTimeout.
      await dp.Runtime.evaluate({expression: 'worker.postMessage(1)'});
      postMessageToWorker = true;
      testRunner.log('Did post message to worker');
    }

    if (postMessageToWorker && message.method === 'Debugger.scriptParsed') {
      var sourceUrl = message.params.url;
      if (!sourceUrl)
        testRunner.log('SUCCESS: script created from string parameter of setTimeout has no url');
      else
        testRunner.log('FAIL: script created from string parameter of setTimeout has url ' + sourceUrl);
      testRunner.completeTest();
    }
  });
})
