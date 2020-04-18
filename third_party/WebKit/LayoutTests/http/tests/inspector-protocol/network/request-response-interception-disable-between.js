(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests to ensure that requests complete if interception disables between request and response.`);

  session.protocol.Network.onRequestIntercepted(async event => {
    testRunner.log('Request Intercepted: ' + event.params.request.url.split('/').pop());
    testRunner.log('Continuing request');
    session.protocol.Network.continueInterceptedRequest({interceptionId: event.params.interceptionId});
    testRunner.log('Disabling request interception');
    await session.protocol.Network.setRequestInterception({patterns: []});
    testRunner.log('');
  });

  await session.protocol.Network.clearBrowserCookies();
  await session.protocol.Network.clearBrowserCache();
  await session.protocol.Network.setCacheDisabled({cacheDisabled: true});
  session.protocol.Network.enable();
  testRunner.log('Network agent enabled');
  await session.protocol.Network.setRequestInterception({patterns: [
      {urlPattern: "*", interceptionStage: 'Request'},
      {urlPattern: "*", interceptionStage: 'HeadersReceived'}
  ]});

  var responseContent = await session.evaluateAsync(`fetch('/devtools/network/resources/resource.php?size=10').then(response => response.text())`);
  testRunner.log('Body: ');
  testRunner.log(responseContent);

  testRunner.completeTest();
})
