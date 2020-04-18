(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests frame lifetime events.');

  dp.Page.enable();
  dp.Network.enable();
  dp.Network.onRequestWillBeSent(e => {
    testRunner.log(`RequestWillBeSent ${testRunner.trimURL(e.params.request.url)}`);
  });
  session.evaluate(`
    window.frame = document.createElement('iframe');
    frame.src = '${testRunner.url('../resources/blank.html')}';
    document.body.appendChild(frame);
  `);
  await dp.Page.onceFrameAttached();
  testRunner.log('Attached');
  await dp.Page.onceFrameStartedLoading();
  testRunner.log('Started loading');
  await dp.Page.onceFrameNavigated();
  testRunner.log('Navigated');
  session.evaluate('frame.src = "about:blank"');
  await dp.Page.onceFrameStartedLoading();
  testRunner.log('Started loading');
  await dp.Page.onceFrameNavigated();
  testRunner.log('Navigated');
  session.evaluate('document.body.removeChild(frame);');
  await dp.Page.onceFrameDetached();
  testRunner.log('Detached');
  testRunner.completeTest();
})
