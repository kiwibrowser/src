(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests line and column numbers in reported console messages.`);

  dp.Runtime.enable();

  dp.Runtime.evaluate({ expression: 'console.log(239)' });
  testRunner.log(await dp.Runtime.onceConsoleAPICalled());
  dp.Runtime.evaluate({ expression: 'var l = console.log;\n  l(239)' });
  testRunner.log(await dp.Runtime.onceConsoleAPICalled());
  testRunner.completeTest();
})
