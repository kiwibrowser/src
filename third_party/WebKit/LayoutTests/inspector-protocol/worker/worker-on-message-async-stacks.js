(async function(testRunner) {
  var {page, session, dp} =
      await testRunner.startBlank('Async stack trace for worker.onmessage.');
  let debuggers = new Map();

  await dp.Target.setAutoAttach(
      {autoAttach: true, waitForDebuggerOnStart: true});

  testRunner.log('Setup page session');
  let pageDebuggerId = (await dp.Debugger.enable()).result.debuggerId;
  debuggers.set(pageDebuggerId, dp.Debugger);
  dp.Debugger.setAsyncCallStackDepth({maxDepth: 32});
  session.evaluate(`
var blob = new Blob(['postMessage(239);//# sourceURL=worker.js'], {type: 'application/javascript'});
var worker = new Worker(URL.createObjectURL(blob));
worker.onmessage = (e) => console.log(e.data);
//# sourceURL=test.js`);

  testRunner.log('Setup worker session');
  let {params: {sessionId}} = await dp.Target.onceAttachedToTarget();
  let wc = new WorkerProtocol(dp, sessionId);
  let workerDebuggerId = (await wc.dp.Debugger.enable()).debuggerId;
  debuggers.set(workerDebuggerId, wc.dp.Debugger);
  wc.dp.Debugger.setAsyncCallStackDepth({maxDepth: 32});

  testRunner.log('Set breakpoint before postMessage');
  wc.dp.Debugger.setBreakpointByUrl(
      {url: 'worker.js', lineNumber: 0, columnNumber: 0});
  testRunner.log('Run worker');
  wc.dp.Runtime.runIfWaitingForDebugger();

  testRunner.log('Run stepInto with breakOnAsyncCall flag');
  await wc.dp.Debugger.oncePaused();

  wc.dp.Debugger.stepInto({breakOnAsyncCall: true});
  testRunner.log('Get scheduledAsyncStackId');
  let {asyncCallStackTraceId} = await wc.dp.Debugger.oncePaused();
  testRunner.log('Request pause on async task and resume');
  dp.Debugger.pauseOnAsyncCall({parentStackTraceId: asyncCallStackTraceId});
  wc.dp.Debugger.resume();

  let {params: {callFrames, asyncStackTrace, asyncStackTraceId}} =
      await dp.Debugger.oncePaused();
  await testRunner.logStackTrace(
      debuggers,
      {callFrames, parent: asyncStackTrace, parentId: asyncStackTraceId},
      pageDebuggerId);
  testRunner.completeTest();
})
