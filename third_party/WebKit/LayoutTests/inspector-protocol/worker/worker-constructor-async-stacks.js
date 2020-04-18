(async function(testRunner) {
  var {page, session, dp} =
      await testRunner.startBlank('Async stack trace in worker constructor.');
  let debuggers = new Map();

  await dp.Target.setAutoAttach(
      {autoAttach: true, waitForDebuggerOnStart: true});

  testRunner.log('Setup page session');
  let pageDebuggerId = (await dp.Debugger.enable()).result.debuggerId;
  debuggers.set(pageDebuggerId, dp.Debugger);
  dp.Debugger.setAsyncCallStackDepth({maxDepth: 32});
  testRunner.log('Set breakpoint before worker created');
  dp.Debugger.setBreakpointByUrl(
      {url: 'test.js', lineNumber: 2, columnNumber: 13});
  session.evaluate(`
var blob = new Blob(['console.log(239);//# sourceURL=worker.js'], {type: 'application/javascript'});
var worker = new Worker(URL.createObjectURL(blob));
//# sourceURL=test.js`);

  testRunner.log('Run stepInto with breakOnAsyncCall flag');
  await dp.Debugger.oncePaused();
  dp.Debugger.stepInto({breakOnAsyncCall: true});
  testRunner.log('Get scheduledAsyncStackId');
  let {params: {asyncCallStackTraceId}} = await dp.Debugger.oncePaused();
  dp.Debugger.resume();

  testRunner.log('Setup worker session');
  let {params: {sessionId}} = await dp.Target.onceAttachedToTarget();
  let wc = new WorkerProtocol(dp, sessionId);

  let workerDebuggerId = (await wc.dp.Debugger.enable()).debuggerId;
  debuggers.set(workerDebuggerId, wc.dp.Debugger);
  wc.dp.Debugger.setAsyncCallStackDepth({maxDepth: 32});
  testRunner.log('Request pause on async task and run worker');
  wc.dp.Debugger.pauseOnAsyncCall({parentStackTraceId: asyncCallStackTraceId});
  wc.dp.Runtime.runIfWaitingForDebugger();

  let {callFrames, asyncStackTrace, asyncStackTraceId} =
      await wc.dp.Debugger.oncePaused();
  await testRunner.logStackTrace(
      debuggers,
      {callFrames, parent: asyncStackTrace, parentId: asyncStackTraceId},
      workerDebuggerId);
  testRunner.completeTest();
})
