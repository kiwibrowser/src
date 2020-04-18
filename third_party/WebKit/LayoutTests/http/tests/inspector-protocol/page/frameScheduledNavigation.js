(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests frameScheduledNavigation events when navigation is initiated in JS followed by other navigations.');

  dp.Page.enable();
  session.evaluate(`
    var frame = document.createElement('iframe');
    document.body.appendChild(frame);
    frame.src = '${testRunner.url('resources/navigation-chain1.html')}';
  `);

  for (var i = 0; i < 6; i++) {
    var msg = await dp.Page.onceFrameScheduledNavigation();
    testRunner.log('Scheduled navigation with delay ' + msg.params.delay +
                   ' and reason ' + msg.params.reason + ' to url ' +
                   msg.params.url.split('/').pop());
    await dp.Page.onceFrameStartedLoading();
    // This event should be received before the scheduled navigation is cleared.
    testRunner.log('Started loading');

    await dp.Page.onceFrameClearedScheduledNavigation();
    testRunner.log('Cleared scheduled navigation');
  }

  testRunner.completeTest();
})
