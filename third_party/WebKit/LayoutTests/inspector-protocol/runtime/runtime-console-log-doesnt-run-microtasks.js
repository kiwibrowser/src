(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Check that console.log doesn't run microtasks.`);

  dp.Runtime.onConsoleAPICalled(result => {
    testRunner.log(result.params.args[0]);
    if (result.params.args[0].value === 'finished')
      testRunner.completeTest();
  });

  dp.Runtime.enable();
  dp.Runtime.evaluate({expression: `
    (function testFunction() {
      Promise.resolve().then(function(){ console.log(239); });
      console.log(42);
      console.log(43);
    })()
  `});
  dp.Runtime.evaluate({expression: `setTimeout(() => console.log('finished'), 0)`});
})
