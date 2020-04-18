(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that Runtime.awaitPromise works.');

  function dumpResult(result) {
    if (result.exceptionDetails) {
      if (result.exceptionDetails.stackTrace && result.exceptionDetails.stackTrace.parent) {
        for (var frame of result.exceptionDetails.stackTrace.parent.callFrames) {
          frame.scriptId = 0;
          frame.url = '';
        }
      }
      result.exceptionDetails.exceptionId = 0;
      if (result.exceptionDetails.exception)
        result.exceptionDetails.exception.objectId = 0;
    }
    testRunner.log(result);
  }

  await session.evaluate(`
    var resolveCallback;
    var rejectCallback;
    function createPromise() {
      return new Promise((resolve, reject) => { resolveCallback = resolve; rejectCallback = reject });
    }

    function resolvePromise() {
      resolveCallback(239);
      resolveCallback = undefined;
      rejectCallback = undefined;
    }

    function rejectPromise() {
      rejectCallback(239);
      resolveCallback = undefined;
      rejectCallback = undefined;
    }

    function runGC() {
      if (window.gc)
        window.gc();
    }
  `);

  await dp.Debugger.enable();
  await dp.Debugger.setAsyncCallStackDepth({ maxDepth: 128 });

  await testRunner.runTestSuite([
    async function testResolvedPromise() {
      var result = await dp.Runtime.evaluate({ expression: 'Promise.resolve(239)'});
      result = await dp.Runtime.awaitPromise({ promiseObjectId: result.result.result.objectId, returnByValue: false, generatePreview: true });
      dumpResult(result.result);
    },

    async function testRejectedPromise() {
      var result = await dp.Runtime.evaluate({ expression: 'Promise.reject({ a : 1 })'});
      result =  await dp.Runtime.awaitPromise({ promiseObjectId: result.result.result.objectId, returnByValue: true, generatePreview: false });
      dumpResult(result.result);
    },

    async function testRejectedPromiseWithStack() {
      var result = await dp.Runtime.evaluate({ expression: 'createPromise()'});
      var promise = dp.Runtime.awaitPromise({ promiseObjectId: result.result.result.objectId });
      dp.Runtime.evaluate({ expression: 'rejectPromise()' });
      result = await promise;
      dumpResult(result.result);
    },

    async function testPendingPromise() {
      var result = await dp.Runtime.evaluate({ expression: 'createPromise()'});
      var promise = dp.Runtime.awaitPromise({ promiseObjectId: result.result.result.objectId });
      dp.Runtime.evaluate({ expression: 'resolvePromise()' });
      result = await promise;
      dumpResult(result.result);
    },

    async function testResolvedWithoutArgsPromise() {
      var result = await dp.Runtime.evaluate({ expression: 'Promise.resolve()'});
      result = await dp.Runtime.awaitPromise({ promiseObjectId: result.result.result.objectId, returnByValue: true, generatePreview: false });
      dumpResult(result.result);
    },

    async function testGarbageCollectedPromise() {
      var result = await dp.Runtime.evaluate({ expression: 'new Promise(() => undefined)'});
      var objectId = result.result.result.objectId;
      var promise = dp.Runtime.awaitPromise({ promiseObjectId: objectId });
      dp.Runtime.releaseObject({ objectId: objectId}).then(() => dp.Runtime.evaluate({ expression: 'runGC()' }));
      result = await promise;
      testRunner.log(result.error);
    }
  ]);
})
