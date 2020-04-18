(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that accessing old call frame does not work.');

  function logErrorResponse(response) {
    if (response.error) {
      if (response.error.message.indexOf('Can only perform operation while paused.') != -1) {
        testRunner.log('PASS, error message as expected');
        return;
      }
    }
    testRunner.log('FAIL, unexpected error message');
    testRunner.log(JSON.stringify(response));
  }

  dp.Debugger.enable();
  dp.Runtime.evaluate({expression: 'setTimeout(() => { debugger; }, 0)' });

  var messageObject = await dp.Debugger.oncePaused();
  testRunner.log(`Paused on 'debugger;'`);
  var topFrame = messageObject.params.callFrames[0];
  var obsoleteTopFrameId = topFrame.callFrameId;

  await dp.Debugger.resume();
  testRunner.log('resume');
  testRunner.log('restartFrame');

  logErrorResponse(await dp.Debugger.restartFrame({callFrameId: obsoleteTopFrameId}));
  testRunner.log('evaluateOnFrame');

  logErrorResponse(await dp.Debugger.evaluateOnCallFrame({callFrameId: obsoleteTopFrameId, expression: '0'}));
  testRunner.log('setVariableValue');

  logErrorResponse(await dp.Debugger.setVariableValue({callFrameId: obsoleteTopFrameId, scopeNumber: 0, variableName: 'a', newValue: { value: 0 }}));
  testRunner.completeTest();
})
