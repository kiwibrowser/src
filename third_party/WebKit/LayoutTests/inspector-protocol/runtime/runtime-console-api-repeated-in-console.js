(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Check that console.log is reported through Console domain as well.`);

  var expectedMessages = 4;
  var messages = [];

  function done() {
    messages.sort();
    for (var message of messages)
      testRunner.log(message);
    testRunner.completeTest();
  }

  dp.Runtime.onConsoleAPICalled(result => {
    messages.push('api call: ' + result.params.args[0].value);
    if (!(--expectedMessages))
      done();
  });

  dp.Console.onMessageAdded(result => {
    messages.push('console message: ' + result.params.message.text);
    if (!(--expectedMessages))
      done();
  });

  dp.Runtime.enable();
  dp.Console.enable();
  dp.Runtime.evaluate({ 'expression': 'console.log(42)' });
  dp.Runtime.evaluate({ 'expression': `console.error('abc')` });
})
