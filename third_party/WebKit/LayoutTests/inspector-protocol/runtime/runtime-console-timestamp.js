(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests timestamp field in reported console messages.`);

  var messages = [];
  dp.Runtime.onConsoleAPICalled(data => {
    var payload = data.params;
    if (messages.length > 0)
      testRunner.log('Message ' + messages.length + ' has non-decreasing timestamp: ' + (payload.timestamp >= messages[messages.length - 1].timestamp));
    messages.push(payload);
    testRunner.log('Message has timestamp: ' + !!payload.timestamp);
    testRunner.log(`Message timestamp doesn't differ too much from current time (one minute interval): ` + (Math.abs(new Date().getTime() - payload.timestamp) < 60000));
    if (messages.length === 3)
      testRunner.completeTest();
  });
  dp.Runtime.enable();
  dp.Runtime.evaluate({ expression: `console.log('testUnique'); for (var i = 0; i < 2; ++i) console.log('testDouble');` });
})
