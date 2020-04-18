(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests interception of an XHR request.`);

  var InterceptionHelper = await testRunner.loadScript('../resources/interception-test.js');
  var helper = new InterceptionHelper(testRunner, session);

  var requestInterceptedDict = {
    'xhr-iframe.html': event => helper.allowRequest(event),
    'example.txt': event => helper.mockResponse(event, 'HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nPayload for the Mock XHR response;'),
  };

  await helper.startInterceptionTest(requestInterceptedDict, 1);
  session.evaluate(`
    var iframe = document.createElement('iframe');
    iframe.src = '${testRunner.url('./resources/xhr-iframe.html')}';
    document.body.appendChild(iframe);
  `);
})
