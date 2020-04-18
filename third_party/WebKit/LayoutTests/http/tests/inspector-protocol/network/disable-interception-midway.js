(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests interception blocking, modification of network fetches.`);

  var InterceptionHelper = await testRunner.loadScript('../resources/interception-test.js');
  var helper = new InterceptionHelper(testRunner, session);

  var requestInterceptedDict = {
      'disable-iframe.html': event => helper.allowRequest(event),
      'i-dont-exist.css': event => helper.disableRequestInterception(event),
      'post-echo.pl': event => helper.allowRequest(event),
  };

  await helper.startInterceptionTest(requestInterceptedDict, 1);
  session.evaluate(`
    var iframe = document.createElement('iframe');
    iframe.src = '${testRunner.url('./resources/disable-iframe.html')}';
    document.body.appendChild(iframe);
  `);
})
