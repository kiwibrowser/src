(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`This tests how navigation is handled from inside debugger code (console.log).`);

  await session.evaluateAsync(`
    function appendIframe(url) {
      var frame = document.createElement('iframe');
      frame.id = 'iframe';
      frame.src = url;
      document.body.appendChild(frame);
      return new Promise(resolve => frame.onload = resolve);
    }
    appendIframe('${testRunner.url('../resources/console-log-navigate-on-load.html')}')
  `);

  await dp.Runtime.enable();
  await checkExpression('logArray()');
  await checkExpression('logDate()');
  await checkExpression('logDateWithArg()');
  testRunner.completeTest();

  async function checkExpression(expression) {
    var contextId;
    dp.Runtime.onceExecutionContextCreated().then(result => contextId = result.params.context.id);
    await session.evaluateAsync(`appendIframe('${testRunner.url('../resources/console-log-navigate.html')}')`);
    testRunner.log(`Got new context: ${contextId !== undefined}`);
    testRunner.log(await dp.Runtime.evaluate({ expression: expression, contextId: contextId }));
  }
})
