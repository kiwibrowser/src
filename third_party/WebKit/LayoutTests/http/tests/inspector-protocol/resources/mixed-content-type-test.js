(async function mixedContentTest(testRunner, session, addIframeWithMixedContent) {
  await session.evaluate(`testRunner.overridePreference('WebKitAllowRunningInsecureContent', true)`);
  await session.protocol.Network.enable();
  testRunner.log('Network agent enabled');
  session.evaluate(addIframeWithMixedContent);

  var numRequests = 0;
  session.protocol.Network.onRequestWillBeSent(event => {
    var req = event.params.request;
    testRunner.log('Mixed content type of ' + req.url + ': ' + req.mixedContentType);
    numRequests++;
    if (numRequests == 2)
      testRunner.completeTest();
  });
})
