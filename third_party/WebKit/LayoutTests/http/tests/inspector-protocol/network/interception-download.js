(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that downloads are correctly identified when intercepting navigation responses.`);

  await session.protocol.Network.clearBrowserCache();
  await session.protocol.Network.setCacheDisabled({cacheDisabled: true});
  await session.protocol.Network.enable();
  await session.protocol.Runtime.enable();

  await dp.Network.setRequestInterception({patterns: [
    {urlPattern: '*', interceptionStage: 'HeadersReceived'}
  ]});

  await dp.Network.onRequestIntercepted(event => {
    const params = event.params;
    testRunner.log(`Intercepted ${params.request.url}, download: ${params.isDownload}`);
    dp.Network.continueInterceptedRequest({interceptionId: params.interceptionId});
  });

  testRunner.log('Regular navigation: ');
  await dp.Page.navigate({url: 'http://127.0.0.1:8000/devtools/network/resources/resource.php'});

  testRunner.log('Download via content-disposition: ');
  await dp.Page.navigate({url: 'http://127.0.0.1:8000/devtools/network/resources/resource.php?download=1'});

  testRunner.log('Download via unhandled MIME type: ');
  await dp.Page.navigate({url: 'http://127.0.0.1:8000/devtools/network/resources/resource.php?mime_type=application/octet-stream'});

  testRunner.log('Now downloading by clicking a link: ');
  session.evaluate(`
    const a = document.createElement('a');
    a.href = '/devtools/network/resources/resource.php';
    a.download = 'hello.text';
    document.body.appendChild(a);
    a.click();
  `);
  await dp.Network.onceRequestIntercepted();

  testRunner.completeTest();
})
