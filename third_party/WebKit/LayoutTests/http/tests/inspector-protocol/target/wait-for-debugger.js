(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that waitForDebuggerOnStart works with out-of-process iframes.`);

  await dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: true});

  await dp.Page.enable();
  dp.Network.enable();
  dp.Network.setUserAgentOverride({userAgent: 'test'});
  session.evaluate(`
    var iframe = document.createElement('iframe');
    iframe.src = 'http://devtools.oopif.test:8000/inspector-protocol/network/resources/echo-headers.php?headers=HTTP_USER_AGENT';
    document.body.appendChild(iframe);
  `);

  let params = (await dp.Network.onceRequestWillBeSent()).params;
  let sessionId = (await dp.Target.onceAttachedToTarget()).params.sessionId;
  let dp1 = session.createChild(sessionId).protocol;
  dp1.Network.enable();
  dp1.Network.setUserAgentOverride({userAgent: 'test (subframe)'});
  dp1.Runtime.runIfWaitingForDebugger();
  testRunner.log(`User-Agent = ${params.request.headers['User-Agent']}`);
  await dp1.Network.onceLoadingFinished();
  let content = (await dp1.Network.getResponseBody({requestId: params.requestId})).result.body;
  testRunner.log(`content (should have user-agent header overriden): ${content}`);

  testRunner.completeTest();
})
