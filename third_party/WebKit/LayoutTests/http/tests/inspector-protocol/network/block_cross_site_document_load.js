(async function(testRunner) {
  var {page, session, dp} = await testRunner.startURL(
      '../resources/test-page.html',
      `Tests the blockedCrossSiteDocumentLoad bit available as a part of the loading finished signal.`);

  const responses = new Map();
  await dp.Network.enable();

  let urls = [
    'http://127.0.0.1:8000/inspector-protocol/network/resources/nosniff.pl',
    'http://127.0.0.1:8000/inspector-protocol/network/resources/simple-iframe.html',
    'http://devtools.oopif.test:8000/inspector-protocol/network/resources/nosniff.pl',
    'http://devtools.oopif.test:8000/inspector-protocol/network/resources/simple-iframe.html',
  ];
  for (const url of urls) {
    session.evaluate(`new Image().src = '${url}';`);
    const response = await dp.Network.onceLoadingFinished();
    testRunner.log(`Blocking cross-site document at ${url}: ${response.params.blockedCrossSiteDocument}`);
  }
  testRunner.completeTest();
})
