(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(`Verify that request interception doesn't mess mime types`);

  await dp.Network.enable();
  await dp.Network.setRequestInterception({
    patterns: [{ urlPattern: '*' }]
  });
  dp.Network.onRequestIntercepted(event => {
    dp.Network.continueInterceptedRequest({
      interceptionId: event.params.interceptionId
    });
  });
  await dp.Runtime.enable();
  await session.navigate('./resources/simple.html');
  // If resource is interpreted as text/plain instead of text/html, the HTML will be escaped.
  testRunner.log(await session.evaluate(() => document.body.innerHTML));
  testRunner.completeTest();
})
