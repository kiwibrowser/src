(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that a request can be intercepted on both request and response stages.`);

  await session.protocol.Network.clearBrowserCookies();
  await session.protocol.Network.clearBrowserCache();
  await session.protocol.Network.setCacheDisabled({cacheDisabled: true});
  await session.protocol.Network.enable();
  await session.protocol.Runtime.enable();

  session.protocol.Network.onRequestWillBeSent(event => {
    testRunner.log(`request will be sent ${event.params.request.url}`);
  });
  session.protocol.Network.onResponseReceived(event => {
    testRunner.log(`response received ${event.params.response.url}`);
  });

  await dp.Network.setRequestInterception({patterns: [
    {urlPattern: '*', interceptionStage: 'Request'},
    {urlPattern: '*', interceptionStage: 'HeadersReceived'}
  ]});

  session.evaluate(`fetch('${testRunner.url('../network/resources/simple-iframe.html')}')`);

  const intercepted1 = (await dp.Network.onceRequestIntercepted()).params;
  testRunner.log(`intercepted request: ${intercepted1.request.url}`);

  dp.Network.continueInterceptedRequest({interceptionId: intercepted1.interceptionId});

  const intercepted2 = (await dp.Network.onceRequestIntercepted()).params;
  testRunner.log(`intercepted response: ${intercepted2.request.url} ${intercepted2.responseStatusCode}`);
  dp.Network.continueInterceptedRequest({interceptionId: intercepted2.interceptionId});

  await dp.Network.onceLoadingFinished();
  testRunner.completeTest();
})
