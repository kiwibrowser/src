(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that navigator.platform can be overridden.');

  await dp.Emulation.setNavigatorOverrides({ platform: 'TestPlatform' });
  var result = await dp.Runtime.evaluate({ expression: 'navigator.platform' });
  testRunner.log('navigator.platform = ' + result.result.result.value);
  testRunner.completeTest();
})
