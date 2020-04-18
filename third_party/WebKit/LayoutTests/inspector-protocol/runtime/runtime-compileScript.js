(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests Runtime.compileScript functionality.`);

  await dp.Debugger.enable();
  dp.Debugger.onScriptParsed(messageObject => {
    if (!messageObject.params.url)
      return;
    testRunner.log('Debugger.scriptParsed: ' + messageObject.params.url);
  });

  dp.Runtime.enable();
  var message = await dp.Runtime.onceExecutionContextCreated();
  var executionContextId = message.params.context.id;
  await testCompileScript('\n  (', false, 'foo1.js');
  await testCompileScript('239', true, 'foo2.js');
  await testCompileScript('239', false, 'foo3.js');
  await testCompileScript('testfunction f()\n{\n    return 0;\n}\n', false, 'foo4.js');
  testRunner.completeTest();

  async function testCompileScript(expression, persistScript, sourceURL) {
    testRunner.log('Compiling script: ' + sourceURL);
    testRunner.log('         persist: ' + persistScript);
    var messageObject = await dp.Runtime.compileScript({
        expression: expression,
        sourceURL: sourceURL,
        persistScript: persistScript,
        executionContextId: executionContextId
    });
    var result = messageObject.result;
    if (result.exceptionDetails) {
      result.exceptionDetails.exceptionId = 0;
      result.exceptionDetails.exception.objectId = 0;
      result.exceptionDetails.scriptId = 0;
    }
    if (result.scriptId)
      result.scriptId = 0;
    testRunner.log(result, 'compilation result: ');
    testRunner.log('-----');
  }
})
