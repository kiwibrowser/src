(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests checks that deprecation messages for console.`);

  var messagesLeft = 3;
  dp.Runtime.onConsoleAPICalled(data => {
    var text = data.params.args[0].value;
    if (text.indexOf('deprecated') === -1)
      return;
    testRunner.log(text);
    if (!--messagesLeft)
      testRunner.completeTest();
  });

  dp.Runtime.enable();
  var deprecatedMethods = [
    `console.timeline('42')`,
    `console.timeline('42')`,
    `console.timeline('42')`, // three calls should produce one warning message
    `console.timelineEnd('42')`,
    `console.markTimeline('42')`,
  ];
  dp.Runtime.evaluate({ expression: deprecatedMethods.join(';') });
})
