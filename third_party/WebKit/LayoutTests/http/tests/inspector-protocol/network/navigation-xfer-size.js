(async function(testRunner) {
  var {page, session, dp} = await testRunner.startURL(
      '../resources/test-page.html',
      `Tests that transfer size is correctly reported for navigations.`);

  dp.Network.enable();
  session.evaluateAsync(`
    var iframe = document.createElement('iframe');
    iframe.src = '/inspector/network/resources/resource.php?gzip=1&size=8000';
    document.body.appendChild(iframe);
  `);
  const response = (await dp.Network.onceResponseReceived()).params.response;

  const encodedLength = (await dp.Network.onceLoadingFinished()).params.encodedDataLength;
  if (encodedLength > 2000)
    testRunner.log(`FAIL: encoded data length is suspiciously large (${encodedLength})`);

  const headersLength = response.headersText.length;
  const contentLength = +response.headers['Content-Length'];
  if (headersLength + contentLength !== encodedLength)
    testRunner.log(`FAIL: headersLength (${headersLength}) + contentLength (${contentLength}) !== encodedLength (${encodedLength})`)
  testRunner.completeTest();
})