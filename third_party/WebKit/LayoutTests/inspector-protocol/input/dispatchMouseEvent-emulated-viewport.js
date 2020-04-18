(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Tests Input.dispatchMouseEvent method after emulating a viewport.`);

  await session.evaluate(`
    window.result = 'Not Clicked';
    window.button = document.createElement('button');
    document.body.appendChild(button);
    button.style.position = 'absolute';
    button.style.top = '50px';
    button.style.left = '50px';
    button.style.width = '10px';
    button.style.height = '10px';
    button.onmousedown = () => window.result = 'Clicked';
  `);

  function dumpError(message) {
    if (message.error)
      testRunner.log('Error: ' + message.error.message);
  }

  dumpError(await dp.Emulation.setDeviceMetricsOverride({
    deviceScaleFactor: 5,
    width: 400,
    height: 300,
    mobile: true
  }));

  dumpError(await dp.Input.dispatchMouseEvent({
    type: 'mousePressed',
    button: 'left',
    clickCount: 1,
    x: 55,
    y: 55
  }));

  testRunner.log(await session.evaluate(`window.result`));
  testRunner.completeTest();
})
