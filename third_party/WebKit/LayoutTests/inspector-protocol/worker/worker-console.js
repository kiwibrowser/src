(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests how console messages from worker get into page's console once worker is destroyed.`);

  await session.evaluate(`
    var worker = null;
    var onMessageCallbacks = {};

    function startWorker() {
      var callback;
      var promise = new Promise((fulfill) => callback = fulfill);
      worker = new Worker('${testRunner.url('../resources/worker-console-worker.js')}');
      worker.onmessage = function(event) {
        worker.onmessage = onMessageFromWorker;
        callback();
      };
      return promise;
    }

    function logInWorkerFromPage(message, callback) {
      onMessageCallbacks[message] = callback;
      worker.postMessage(message);
    }

    function onMessageFromWorker(event) {
      var callback = onMessageCallbacks[event.data];
      delete onMessageCallbacks[event.data];
      if (callback)
        callback();
    }

    function stopWorker() {
      worker.terminate();
      worker = null;
    }
  `);

  var workerEventHandler = {};
  dp.Target.onAttachedToTarget(onWorkerCreated);
  dp.Target.onReceivedMessageFromTarget(onWorkerMessage);
  workerEventHandler['Runtime.consoleAPICalled'] = onConsoleAPICalledFromWorker;

  var workerId;

  function onWorkerCreated(payload) {
    testRunner.log('Worker.created');
    workerId = payload.params.targetInfo.targetId;
  }

  var requestId = 0;
  var dispatchTable = [];

  function sendCommandToWorker(method, params, callback) {
    dispatchTable[++requestId] = callback;
    var messageObject = {
      'method': method,
      'params': params,
      'id': requestId
    };
    dp.Target.sendMessageToTarget({
      targetId: workerId,
      message: JSON.stringify(messageObject)
    });
  }

  function onWorkerMessage(payload) {
    if (payload.params.targetId !== workerId)
      testRunner.log('targetId mismatch');
    var messageObject = JSON.parse(payload.params.message);
    var messageId = messageObject['id'];
    if (typeof messageId === 'number') {
      var handler = dispatchTable[messageId];
      dispatchTable[messageId] = null;
      if (handler && typeof handler === 'function')
        handler(messageObject);
    } else {
      var eventName = messageObject['method'];
      var eventHandler = workerEventHandler[eventName];
      if (eventHandler)
        eventHandler(messageObject);
    }
  }

  function logInWorker(message, next) {
    testRunner.log('Logging in worker: ' + message);
    dp.Log.onEntryAdded(onLogEntry);
    session.evaluate('logInWorkerFromPage(\'' + message + '\')');

    function onLogEntry(payload) {
      testRunner.log('Got log message from page: ' + payload.params.entry.text);
      dp.Log.offEntryAdded(onLogEntry);
      next();
    }
  }

  var gotMessages = [];
  var waitingForMessage;
  var waitingForMessageCallback;

  function onConsoleAPICalledFromWorker(payload) {
    var message = payload.params.args[0].value;
    testRunner.log('Got console API call from worker: ' + message);
    gotMessages.push(message);
    if (message === waitingForMessage)
      waitingForMessageCallback();
  }

  function waitForMessage(message, next) {
    if (gotMessages.indexOf(message) !== -1) {
      next();
      return;
    }
    waitingForMessage = message;
    waitingForMessageCallback = next;
  }

  var steps = [
    function listenToConsole(next) {
      dp.Log.enable().then(next);
    },

    function start0(next) {
      testRunner.log('Starting worker');
      session.evaluateAsync('startWorker()').then(next);
    },

    function log0(next) {
      logInWorker('message0', next);
    },

    function stop0(next) {
      testRunner.log('Stopping worker');
      session.evaluate('stopWorker()').then(next);
    },

    function start1(next) {
      testRunner.log('Starting worker');
      session.evaluateAsync('startWorker()').then(next);
    },

    function log1(next) {
      logInWorker('message1', next);
    },

    function enable1(next) {
      testRunner.log('Starting autoattach');
      dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: false}).then(next);
    },

    function consoleEnable1(next) {
      testRunner.log('Sending Runtime.enable to worker');
      waitForMessage('message1', next);
      sendCommandToWorker('Runtime.enable', {});
    },

    function log2(next) {
      logInWorker('message2', next);
    },

    function waitForMessage2(next) {
      waitForMessage('message2', next);
    },

    function throw1(next) {
      logInWorker('throw1', next);
    },

    function disable1(next) {
      testRunner.log('Stopping autoattach');
      dp.Target.setAutoAttach({autoAttach: false, waitForDebuggerOnStart: false}).then(next);
    },

    function log3(next) {
      logInWorker('message3', next);
    },

    function stop1(next) {
      testRunner.log('Stopping worker');
      session.evaluate('stopWorker()').then(next);
    },

    function enable2(next) {
      testRunner.log('Starting autoattach');
      dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: false}).then(next);
    },

    function start2(next) {
      testRunner.log('Starting worker');
      session.evaluateAsync('startWorker()').then(next);
    },

    function log4(next) {
      logInWorker('message4', next);
    },

    function consoleEnable2(next) {
      testRunner.log('Sending Runtime.enable to worker');
      waitForMessage('message4', next);
      sendCommandToWorker('Runtime.enable', {});
    },

    function log5(next) {
      logInWorker('message5', next);
    },

    function waitForMessage5(next) {
      waitForMessage('message5', next);
    },

    function stop2(next) {
      testRunner.log('Stopping worker');
      session.evaluate('stopWorker()').then(next);
    },

    function start3(next) {
      testRunner.log('Starting worker');
      session.evaluateAsync('startWorker()').then(next);
    },

    function log6(next) {
      logInWorker('message6', next);
    },

    function stop3(next) {
      testRunner.log('Stopping worker');
      session.evaluate('stopWorker()').then(next);
    },

    function disable2(next) {
      testRunner.log('Stopping autoattach');
      dp.Target.setAutoAttach({autoAttach: false, waitForDebuggerOnStart: false}).then(next);
    }
  ];

  function runNextStep() {
    if (!steps.length) {
      testRunner.completeTest();
      return;
    }
    var nextStep = steps.shift();
    nextStep(runNextStep);
  }

  runNextStep();
})
