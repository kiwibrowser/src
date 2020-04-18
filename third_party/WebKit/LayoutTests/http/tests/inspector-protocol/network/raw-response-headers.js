(async function(testRunner) {
  var {page, session, dp} = await testRunner.startURL(
      '../resources/test-page.html',
      `Tests that raw response headers are correctly reported in case of revalidation.`);

  dp.Network.enable();
  session.protocol.Page.reload();
  const response = (await dp.Network.onceResponseReceived()).params.response;

  const haveContentLength = Object.keys(response.headers).indexOf('Content-Length') >= 0;
  testRunner.log(`Response status: ${response.status} ${response.statusText}`);
  testRunner.log(`Response headers text present: ${!!response.headersText.length}`);
  testRunner.log(`Content-Length present: ${haveContentLength}`);

  testRunner.completeTest();
})