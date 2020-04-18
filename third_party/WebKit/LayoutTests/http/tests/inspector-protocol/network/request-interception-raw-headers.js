(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that raw response headers are correctly reported in case of interception.`);

  var InterceptionHelper = await testRunner.loadScript('../resources/interception-test.js');
  var helper = new InterceptionHelper(testRunner, session);

  var requestInterceptedDict = {
    'simple-iframe.html': event => helper.allowRequest(event),
  };

  await helper.startInterceptionTest(requestInterceptedDict);
  session.evaluate(`
    var iframe = document.createElement('iframe');
    iframe.src = '${testRunner.url('./resources/simple-iframe.html')}';
    document.body.appendChild(iframe);
  `);

  await dp.Network.onResponseReceived(event => {
    const response = event.params.response;
    const haveRequestHeadersText = response.requestHeadersText;
    const splitRawHeaders = response.requestHeadersText.split('\r\n');
    const connectionHeaderPresent = splitRawHeaders.filter(header => header.includes('Connection'));

    testRunner.log(`Response.requestHeadersText present: ${!!haveRequestHeadersText}`);
    testRunner.log(`Connection raw header present: ${!!connectionHeaderPresent.length}`);
  });
})